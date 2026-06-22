#include "Faahhder/Scripting.hpp"

#include "Faahhder/Event.hpp"
#include "Faahhder/Input.hpp"
#include "Faahhder/Physics.hpp"

#include <fstream>
#include <sstream>

#if Faahhder_WITH_LUA
extern "C" {
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
}
#endif

namespace faahhder {
namespace {

#if Faahhder_WITH_LUA
Scene* g_luaScene = nullptr;

int LuaLogInfo(lua_State* state) {
    const char* message = luaL_checkstring(state, 1);
    Events::Emit({"Log", -1, message ? message : ""});
    return 0;
}

int LuaInputIsKeyDown(lua_State* state) {
    const char* key = luaL_checkstring(state, 1);
    lua_pushboolean(state, key && Input::IsKeyDown(key));
    return 1;
}

int LuaEntityGetX(lua_State* state) {
    const auto entity = static_cast<EntityId>(luaL_checkinteger(state, 1));
    const auto* transform = g_luaScene ? g_luaScene->GetTransform(entity) : nullptr;
    lua_pushnumber(state, transform ? transform->position.x : 0.0f);
    return 1;
}

int LuaEntityGetY(lua_State* state) {
    const auto entity = static_cast<EntityId>(luaL_checkinteger(state, 1));
    const auto* transform = g_luaScene ? g_luaScene->GetTransform(entity) : nullptr;
    lua_pushnumber(state, transform ? transform->position.y : 0.0f);
    return 1;
}

int LuaEntitySetPosition(lua_State* state) {
    const auto entity = static_cast<EntityId>(luaL_checkinteger(state, 1));
    const float x = static_cast<float>(luaL_checknumber(state, 2));
    const float y = static_cast<float>(luaL_checknumber(state, 3));
    auto* transform = g_luaScene ? g_luaScene->GetTransform(entity) : nullptr;
    if (transform) {
        transform->position = {x, y};
    }
    return 0;
}

int LuaSceneSpawn(lua_State* state) {
    const char* name = luaL_optstring(state, 1, "Lua Entity");
    const auto entity = g_luaScene ? g_luaScene->CreateEntity(name) : InvalidEntity;
    lua_pushinteger(state, static_cast<lua_Integer>(entity));
    return 1;
}

int LuaPhysicsRaycast(lua_State* state) {
    const Vec2 origin{static_cast<float>(luaL_checknumber(state, 1)), static_cast<float>(luaL_checknumber(state, 2))};
    const Vec2 direction{static_cast<float>(luaL_checknumber(state, 3)), static_cast<float>(luaL_checknumber(state, 4))};
    const float distance = static_cast<float>(luaL_checknumber(state, 5));
    const auto hit = g_luaScene ? Physics2D::Raycast(*g_luaScene, origin, direction, distance) : std::nullopt;
    lua_pushboolean(state, hit.has_value());
    if (hit) {
        lua_pushinteger(state, static_cast<lua_Integer>(hit->entity));
        return 2;
    }
    return 1;
}

void BindBasicApi(lua_State* state, Scene& scene) {
    g_luaScene = &scene;

    lua_newtable(state);
    lua_pushcfunction(state, LuaLogInfo);
    lua_setfield(state, -2, "Info");
    lua_setglobal(state, "Log");

    lua_newtable(state);
    lua_pushcfunction(state, LuaInputIsKeyDown);
    lua_setfield(state, -2, "IsKeyDown");
    lua_setglobal(state, "Input");

    lua_newtable(state);
    lua_pushcfunction(state, LuaEntityGetX);
    lua_setfield(state, -2, "GetX");
    lua_pushcfunction(state, LuaEntityGetY);
    lua_setfield(state, -2, "GetY");
    lua_pushcfunction(state, LuaEntitySetPosition);
    lua_setfield(state, -2, "SetPosition");
    lua_setglobal(state, "Entity");

    lua_newtable(state);
    lua_pushcfunction(state, LuaSceneSpawn);
    lua_setfield(state, -2, "Spawn");
    lua_setglobal(state, "Scene");

    lua_newtable(state);
    lua_pushcfunction(state, LuaPhysicsRaycast);
    lua_setfield(state, -2, "Raycast");
    lua_setglobal(state, "Physics");
}
#endif

}

void ScriptingEngine::BindRuntimeApi(Scene* scene) {
    if (m_scene == scene) {
        return;
    }
    m_scene = scene;
    m_log.push_back("Lua API bound: Entity, Input, Scene, Physics, Log");
}

void ScriptingEngine::OnCreate(Scene& scene, EntityId entity) {
    ExecuteLifecycle(scene, entity, "OnCreate");
}

void ScriptingEngine::OnUpdate(Scene& scene, EntityId entity, float dt) {
    ExecuteLifecycle(scene, entity, "OnUpdate", dt);
}

void ScriptingEngine::OnDestroy(Scene& scene, EntityId entity) {
    ExecuteLifecycle(scene, entity, "OnDestroy");
}

void ScriptingEngine::OnCollisionEnter(Scene& scene, EntityId entity, EntityId other) {
    ExecuteLifecycle(scene, entity, "OnCollisionEnter", 0.0f, other);
}

void ScriptingEngine::OnTriggerEnter(Scene& scene, EntityId entity, EntityId other) {
    ExecuteLifecycle(scene, entity, "OnTriggerEnter", 0.0f, other);
}

void ScriptingEngine::HotReload(Scene& scene) {
    for (auto entity : scene.Entities()) {
        auto* script = scene.GetLuaScript(entity);
        if (!script || !script->hotReload || script->path.empty() || !std::filesystem::exists(script->path)) {
            continue;
        }
        const auto writeTime = std::filesystem::last_write_time(script->path);
        if (script->lastWriteTime != std::filesystem::file_time_type{} && script->lastWriteTime != writeTime) {
            m_log.push_back("Hot reloaded " + script->path.string());
            OnCreate(scene, entity);
        }
        script->lastWriteTime = writeTime;
    }
}

const std::vector<std::string>& ScriptingEngine::Log() const {
    return m_log;
}

void ScriptingEngine::ExecuteLifecycle(Scene& scene, EntityId entity, const std::string& lifecycle, float dt, EntityId other) {
    const auto* script = scene.GetLuaScript(entity);
    const auto* info = scene.GetInfo(entity);
    if (!script || !info) {
        return;
    }

    std::ifstream file(script->path);
    if (!file) {
        m_log.push_back("Missing script: " + script->path.string());
        return;
    }

#if Faahhder_WITH_LUA
    lua_State* state = luaL_newstate();
    luaL_openlibs(state);
    BindBasicApi(state, scene);

    if (luaL_dofile(state, script->path.string().c_str()) != LUA_OK) {
        m_log.push_back("Lua error: " + std::string(lua_tostring(state, -1)));
        lua_close(state);
        return;
    }

    lua_getglobal(state, lifecycle.c_str());
    if (lua_isfunction(state, -1)) {
        lua_pushinteger(state, static_cast<lua_Integer>(entity));
        if (lifecycle == "OnUpdate") {
            lua_pushnumber(state, dt);
        } else if (lifecycle == "OnCollisionEnter" || lifecycle == "OnTriggerEnter") {
            lua_pushinteger(state, static_cast<lua_Integer>(other));
        }

        const int args = lifecycle == "OnUpdate" || lifecycle == "OnCollisionEnter" || lifecycle == "OnTriggerEnter" ? 2 : 1;
        if (lua_pcall(state, args, 0, 0) != LUA_OK) {
            m_log.push_back("Lua error: " + std::string(lua_tostring(state, -1)));
        } else {
            m_log.push_back(lifecycle + "(" + info->name + ")");
        }
    }

    lua_close(state);
#else
    std::stringstream source;
    source << file.rdbuf();
    if (source.str().find("function " + lifecycle) != std::string::npos) {
        m_log.push_back(lifecycle + "(" + info->name + ", dt=" + std::to_string(dt) + ", other=" + std::to_string(other) + ")");
    }
#endif
}

}
