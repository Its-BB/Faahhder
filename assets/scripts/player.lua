function OnCreate(entity)
    Log.Info("Player created")
end

function OnUpdate(entity, dt)
    if Input.IsKeyDown("A") then
        Entity.SetPosition(entity, Entity.GetX(entity) - 120 * dt, Entity.GetY(entity))
    end
    if Input.IsKeyDown("D") then
        Entity.SetPosition(entity, Entity.GetX(entity) + 120 * dt, Entity.GetY(entity))
    end
end

function OnCollisionEnter(entity, other)
    Log.Info("Player collision")
end
