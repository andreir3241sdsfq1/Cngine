-- example_script.lua — Cngine v2
-- Пример скрипта: вызывается из game.c через ScriptManager
-- Доступные модули: engine, render, audio, phys, Key

local player = { x = 160, y = 100, speed = 80 }
local bg_color = { 20, 20, 40 }
local time = 0

-- Вызывается один раз при загрузке
function on_init()
    print("[Lua] on_init called")
    render.set_clear_color(bg_color[1], bg_color[2], bg_color[3])

    -- Зарегистрировать звуковые события
    -- audio.register_event("jump", "jump_sfx", 0.8, 1.0, 0.9, 1.1, 0.1)
    -- audio.play_music("main_theme", 0.7, true)

    -- Включить пост-эффекты
    render.postfx_vignette(true, 0.4)
    render.postfx_grain(true, 0.025)
end

-- Вызывается каждый кадр
function on_update(dt)
    time = time + dt

    local spd = player.speed * dt
    if engine.key_down(Key.A) or engine.key_down(Key.LEFT) then
        player.x = player.x - spd
    end
    if engine.key_down(Key.D) or engine.key_down(Key.RIGHT) then
        player.x = player.x + spd
    end
    if engine.key_down(Key.W) or engine.key_down(Key.UP) then
        player.y = player.y - spd
    end
    if engine.key_down(Key.S) or engine.key_down(Key.DOWN) then
        player.y = player.y + spd
    end

    -- Пример триггера звука по нажатию
    if engine.key_pressed(Key.SPACE) then
        -- audio.trigger("jump")
        render.camera_shake(0.3)
    end

    -- Ограничить в пределах экрана
    local w, h = engine.width(), engine.height()
    if player.x < 5 then player.x = 5 end
    if player.x > w-5 then player.x = w-5 end
    if player.y < 5 then player.y = 5 end
    if player.y > h-5 then player.y = h-5 end
end

-- Вызывается после рендера сцены
function on_render()
    local glow_r = 100 + math.floor(math.sin(time*2)*55)
    local glow_g = 200
    local glow_b = 255

    -- Нарисовать игрока (светящийся кружок с bloom-тегом)
    render.draw_circle(player.x, player.y, 6, glow_r, glow_g, glow_b, 255, true)
    render.draw_circle(player.x, player.y, 8, glow_r, glow_g, glow_b, 80, false)

    -- HUD
    local fps = engine.fps()
    local dt  = engine.dt()
    render.draw_text("FPS: " .. fps, 4, 4, 255, 255, 100, 255, 1.0)
    render.draw_text("WASD to move, SPACE to shake", 4, 14, 180, 180, 180, 255, 1.0)
    render.draw_text(string.format("x=%.1f y=%.1f", player.x, player.y), 4, 24, 100, 220, 255, 255, 1.0)
end

-- Вызывается при SDL-событии (строка типа "SDL_KEYDOWN" и т.п.)
function on_event(event_type)
    if event_type == "SDL_QUIT" then
        engine.quit()
    end
end

-- Вызывается при выгрузке скрипта
function on_destroy()
    print("[Lua] on_destroy called")
end
