function OnCreate(entity)
    Log.Info("Player ready")
end

function OnUpdate(entity, dt)
    local speed = 120
    if Input.IsKeyDown("A") then
        Entity.SetPosition(entity, Entity.GetX(entity) - speed * dt, Entity.GetY(entity))
    end
    if Input.IsKeyDown("D") then
        Entity.SetPosition(entity, Entity.GetX(entity) + speed * dt, Entity.GetY(entity))
    end
end
