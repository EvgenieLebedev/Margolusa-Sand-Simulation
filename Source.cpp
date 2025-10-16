// Блочный клеточный автомат Марголуса — реализация правила «песка» с использованием SFML
// Пример компиляции:
// g++ -std=c++17 margolus_sand_sfml.cpp -o margolus -lsfml-graphics -lsfml-window -lsfml-system

#include <SFML/Graphics.hpp>
#include <vector>
#include <array>
#include <random>
#include <string>
#include <iostream>
#pragma execution_character_set("utf-8")

// Размеры сетки должны быть кратны 2 по обеим осям
const int CELL_SIZE = 6; // размер ячейки в пикселях
const int GRID_W = 160;  // ширина сетки в ячейках (должна быть чётной)
const int GRID_H = 120;  // высота сетки в ячейках (должна быть чётной)

using Block = std::array<int, 4>; // порядок: левый верхний, правый верхний, левый нижний, правый нижний

// Вспомогательная функция: горизонтальное отражение блока
Block mirror_h(const Block& b) {
    return Block{ b[1], b[0], b[3], b[2] };
}

// Проверка, соответствует ли блок шаблону правила rule_pat
// Значение -1 в шаблоне означает «подходит любое значение»
bool match_pattern(const Block& rule_pat, const Block& b) {
    for (int i = 0; i < 4; ++i) {
        if (rule_pat[i] == -1) continue;
        if (rule_pat[i] != b[i]) return false;
    }
    return true;
}

// Генерация выходного блока по шаблону: -1 означает «скопировать значение из входа»
Block apply_output_template(const Block& out_tpl, const Block& in) {
    Block out;
    for (int i = 0; i < 4; ++i) {
        if (out_tpl[i] == -1) out[i] = in[i];
        else out[i] = out_tpl[i];
    }
    return out;
}

// Правило автомата: шаблон входа, шаблон выхода и флаг симметрии по горизонтали
struct Rule {
    Block in;
    Block out;
    bool horizontal_reflection = false;
};

// Формирование набора правил для модели «песка»
std::vector<Rule> build_sand_rules() {
    // Значение -1 используется для переменных a и b (означает «любое значение»)
    std::vector<Rule> rules;

    // Симметрия по горизонтали (для зеркальных вариантов)
    // 1, 1, 0, 0 → 0, 0, 1, 1
    rules.push_back(Rule{ Block{1,1,0,0}, Block{0,0,1,1}, true });

    // 1, a, 0, b → 0, a, 1, b  (a,b — переменные)
    rules.push_back(Rule{ Block{1,-1,0,-1}, Block{0,-1,1,-1}, true });

    // 1, 0, a, 0 → 0, 0, a, 1
    rules.push_back(Rule{ Block{1,0,-1,0}, Block{0,0,-1,1}, true });

    // 3, a, 0, b → 3, a, 1, b
    rules.push_back(Rule{ Block{3,-1,0,-1}, Block{3,-1,1,-1}, true });

    return rules;
}

// Класс автомата Марголуса
struct Margolus {
    int w, h;                // размеры сетки в ячейках
    std::vector<int> cells;  // состояние ячеек (значения 0..3)
    bool offset = false;     // смещение блока (чередуется каждый шаг)
    std::vector<Rule> rules; // набор правил

    Margolus(int W, int H) : w(W), h(H), cells(W* H, 0) {
        rules = build_sand_rules();
    }

    int& at(int x, int y) { x = (x % w + w) % w; y = (y % h + h) % h; return cells[y * w + x]; }

    // Один шаг автомата
    void step() {
        std::vector<int> next = cells; // начнем с текущих значений

        int ox = offset ? 1 : 0;
        int oy = offset ? 1 : 0; // диагональное смещение (1,1), когда offset == true

        for (int by = oy; by < h + oy; by += 2) {
            for (int bx = ox; bx < w + ox; bx += 2) {
                // координаты левого верхнего угла блока (с учетом зацикливания)
                int x0 = (bx) % w;
                int y0 = (by) % h;
                Block b{ at(x0,y0), at(x0 + 1,y0), at(x0,y0 + 1), at(x0 + 1,y0 + 1) };

                bool applied = false;
                // попытка применить каждое правило
                for (const auto& r : rules) {
                    // прямое совпадение
                    if (match_pattern(r.in, b)) {
                        Block out = apply_output_template(r.out, b);
                        next[y0 * w + x0] = out[0];
                        next[y0 * w + ((x0 + 1) % w)] = out[1];
                        next[((y0 + 1) % h) * w + x0] = out[2];
                        next[((y0 + 1) % h) * w + ((x0 + 1) % w)] = out[3];
                        applied = true;
                        break;
                    }
                    // если правило симметрично — проверяем зеркальную копию
                    if (r.horizontal_reflection) {
                        Block mb = mirror_h(b);
                        if (match_pattern(r.in, mb)) {
                            Block mout = apply_output_template(r.out, mb);
                            Block mirrored_back = mirror_h(mout);
                            next[y0 * w + x0] = mirrored_back[0];
                            next[y0 * w + ((x0 + 1) % w)] = mirrored_back[1];
                            next[((y0 + 1) % h) * w + x0] = mirrored_back[2];
                            next[((y0 + 1) % h) * w + ((x0 + 1) % w)] = mirrored_back[3];
                            applied = true;
                            break;
                        }
                    }
                }
                // если ни одно правило не подошло — блок остаётся без изменений
            }
        }

        cells.swap(next);
        offset = !offset;
    }

    void clear() { std::fill(cells.begin(), cells.end(), 0); }

    void randomize(double fill_prob = 0.12) {
        std::mt19937 rng(12345);
        std::uniform_real_distribution<double> d(0, 1);
        for (int i = 0; i < w * h; ++i) cells[i] = d(rng) < fill_prob ? 1 : 0;
    }
};

// Цвета для состояний: 0 — пусто, 1 — песок, 2 — твёрдая поверхность, 3 — источник
sf::Color color_for_state(int s) {
    switch (s) {
    case 0: return sf::Color(20, 20, 20); // почти чёрный
    case 1: return sf::Color(212, 175, 55); // песочный
    case 2: return sf::Color(100, 40, 20); // грунт
    case 3: return sf::Color(230, 230, 230); // источник
    default: return sf::Color::Magenta;
    }
}

int main() {

    int win_w = GRID_W * CELL_SIZE;
    int win_h = GRID_H * CELL_SIZE;

    sf::RenderWindow window(sf::VideoMode(win_w, win_h), "Margolus: Sand (SFML)");
    window.setFramerateLimit(60);

    Margolus sim(GRID_W, GRID_H);
    sim.randomize(0.09);

    bool running = true;
    float accumulator = 0.f;
    float step_interval = 0.05f; // шаг автомата (секунд на итерацию)

    // Вершинный массив для быстрого рисования
    sf::VertexArray verts(sf::Quads, GRID_W * GRID_H * 4);

    auto update_vertices = [&](void) {
        int idx = 0;
        for (int y = 0; y < GRID_H; ++y) {
            for (int x = 0; x < GRID_W; ++x) {
                sf::Color c = color_for_state(sim.at(x, y));
                float fx = x * CELL_SIZE;
                float fy = y * CELL_SIZE;
                verts[idx + 0].position = sf::Vector2f(fx, fy);
                verts[idx + 1].position = sf::Vector2f(fx + CELL_SIZE, fy);
                verts[idx + 2].position = sf::Vector2f(fx + CELL_SIZE, fy + CELL_SIZE);
                verts[idx + 3].position = sf::Vector2f(fx, fy + CELL_SIZE);
                for (int k = 0; k < 4; ++k) verts[idx + k].color = c;
                idx += 4;
            }
        }
        };

    update_vertices();

    // Текстовая информация
    sf::Font font;
    if (!font.loadFromFile("DejaVuSans.ttf")) {
        // если шрифт не найден — продолжаем без текста
    }
    sf::Text info_text;
    info_text.setFont(font);
    info_text.setCharacterSize(14);
    info_text.setFillColor(sf::Color::White);
    info_text.setPosition(6, 6);

    int brush_state = 1; // состояние, которое рисуется при клике

    sf::Clock clock;

    while (window.isOpen()) {
        sf::Time dt = clock.restart();
        accumulator += dt.asSeconds();

        sf::Event ev;
        while (window.pollEvent(ev)) {
            if (ev.type == sf::Event::Closed) window.close();
            else if (ev.type == sf::Event::KeyPressed) {
                if (ev.key.code == sf::Keyboard::Space) running = !running;
                else if (ev.key.code == sf::Keyboard::S) { sim.step(); update_vertices(); }
                else if (ev.key.code == sf::Keyboard::C) { sim.clear(); update_vertices(); }
                else if (ev.key.code == sf::Keyboard::R) { sim.randomize(0.09); update_vertices(); }
                else if (ev.key.code == sf::Keyboard::Num1) brush_state = 0;
                else if (ev.key.code == sf::Keyboard::Num2) brush_state = 1;
                else if (ev.key.code == sf::Keyboard::Num3) brush_state = 2;
                else if (ev.key.code == sf::Keyboard::Num4) brush_state = 3;
                else if (ev.key.code == sf::Keyboard::Up) step_interval = std::max(0.005f, step_interval - 0.01f);
                else if (ev.key.code == sf::Keyboard::Down) step_interval += 0.01f;
            }
            else if (ev.type == sf::Event::MouseButtonPressed || ev.type == sf::Event::MouseMoved) {
                if (sf::Mouse::isButtonPressed(sf::Mouse::Left)) {
                    sf::Vector2i mp = sf::Mouse::getPosition(window);
                    int gx = mp.x / CELL_SIZE;
                    int gy = mp.y / CELL_SIZE;
                    if (gx >= 0 && gx < GRID_W && gy >= 0 && gy < GRID_H) {
                        sim.at(gx, gy) = brush_state;
                        update_vertices();
                    }
                }
                if (sf::Mouse::isButtonPressed(sf::Mouse::Right)) {
                    sf::Vector2i mp = sf::Mouse::getPosition(window);
                    int gx = mp.x / CELL_SIZE;
                    int gy = mp.y / CELL_SIZE;
                    if (gx >= 0 && gx < GRID_W && gy >= 0 && gy < GRID_H) {
                        // циклическая смена состояния ячейки
                        sim.at(gx, gy) = (sim.at(gx, gy) + 1) % 4;
                        update_vertices();
                    }
                }
            }
        }

        if (running) {
            if (accumulator >= step_interval) {
                int steps = int(accumulator / step_interval);
                accumulator -= steps * step_interval;
                for (int i = 0; i < steps; ++i) sim.step();
                update_vertices();
            }
        }

        // Отрисовка
        window.clear(sf::Color::Black);
        window.draw(verts);

        // Информационная панель
        sf::String info = L"Space: запуск/пауза  S: шаг  C: очистить  R: случайно  1-4: кисть  ЛКМ: рисовать  ПКМ: смена\n";
        info += L"Скорость (Up/Down): " + std::to_wstring(int(1.0f / step_interval)) + L" шагов/сек\n";
        info += L"Состояние кисти: " + std::to_wstring(brush_state) + L" (0 — пусто, 1 — песок, 2 — грунт, 3 — источник)";
        info_text.setString(info);

        if (font.getInfo().family != "") window.draw(info_text);

        window.display();
    }

    return 0;
}
