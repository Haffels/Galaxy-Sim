#include <SFML/Graphics.hpp>
#include <vector>
#include <cmath>
#include <random>
#include <algorithm>

const int NUM_STARS = 88000;
const float GALAXY_SIZE = 220.0f;
const float CORE_SIZE = 55.0f;
const float TIME_STEP = 0.008f;

const int GRID_LINES = 20;
const float GRID_SPACING = 25.0f;
const float GRID_DEPTH = 90.0f;
const float WARP_STRENGTH = 9000.0f;

struct Star {
    float x, y, z;
    float radius, angle, speed;
    sf::Color color;
    bool isCore;
};

sf::Color lerp(sf::Color a, sf::Color b, float t) {
    return sf::Color(
        a.r + (b.r - a.r) * t,
        a.g + (b.g - a.g) * t,
        a.b + (b.b - a.b) * t,
        a.a + (b.a - a.a) * t
    );
}

sf::Vector2f project(float x, float y, float z, float cx, float cy) {
    return { (x - z) * 0.866f + cx, y + (x + z) * 0.5f + cy };
}

int main() {
    sf::RenderWindow window(sf::VideoMode(600, 600), "Galaxy", sf::Style::Titlebar | sf::Style::Close);
    window.setFramerateLimit(60);

    std::mt19937 rng(std::random_device{}());
    std::normal_distribution<float> N(0.0f, 1.0f);
    std::uniform_real_distribution<float> U(0.0f, 1.0f);

    std::vector<Star> stars;
    stars.reserve(NUM_STARS);

    for (int i = 0; i < NUM_STARS; ++i) {
        float rPick = U(rng);
        float x, y, z, r, theta;
        bool core = false;
        sf::Color col;

        if (rPick < 0.22f) {
            core = true;

            x = N(rng) * CORE_SIZE * 1.8f;
            z = N(rng) * CORE_SIZE * 0.6f;
            y = N(rng) * 2.0f;

            float barAngle = 0.35f;
            float tx = x * cos(barAngle) - z * sin(barAngle);
            float tz = x * sin(barAngle) + z * cos(barAngle);
            x = tx; z = tz;

            r = std::sqrt(x*x + z*z);
            theta = atan2(z, x);

            col = lerp({255,255,245,230}, {255,200,160,210}, r / CORE_SIZE);
        }
        else {
            r = U(rng) * GALAXY_SIZE + 10.0f;

            int arms = 4;
            float arm = int(U(rng) * arms) * (2.0f * 3.14159f / arms);

            float spiral = arm + 1.35f * log(r);

            float turbulence =
                sin(r * 0.06f + spiral * 1.3f) * 0.15f +
                N(rng) * 0.12f * (r / GALAXY_SIZE);

            theta = spiral + turbulence;

            if (U(rng) < 0.08f)
                theta += N(rng) * 0.9f;

            float phaseJitter =
                sin(r * 0.03f + N(rng) * 0.6f) +
                sin(theta * 2.3f + r * 0.01f);

            float dust = sin(theta * 3.7f + r * 0.04f + phaseJitter);
            float dustStrength = std::clamp((dust + 1.0f) * 0.5f, 0.0f, 1.0f);

            if (U(rng) > dustStrength * 0.9f + 0.1f)
                continue;

            x = cos(theta) * r + N(rng) * 0.6f;
            z = sin(theta) * r + N(rng) * 0.6f;

            y = std::clamp(N(rng) * (1.2f + r * 0.008f), -8.0f, 8.0f);

            sf::Color warm(245,240,235);
            sf::Color blue(205,215,255);
            sf::Color violet(150,120,190);

            float t = U(rng) * 0.35f;
            col = lerp(warm, blue, t);

            float dustMix = std::clamp(pow(r / GALAXY_SIZE, 1.2f), 0.0f, 1.0f);
            col = lerp(col, violet, dustMix * 0.75f);

            sf::Color dustPurple(140,95,190);
            col = lerp(col, dustPurple, (1.0f - dustStrength) * 0.6f);

            col.r = std::min<sf::Uint8>(col.r, static_cast<sf::Uint8>(200));
            col.g = std::min<sf::Uint8>(col.g, static_cast<sf::Uint8>(190));
            col.a = 165 * dustStrength;
        }

        float speed = 3.5f / (sqrt(r) + 0.6f);
        stars.push_back({x,y,z,r,theta,speed,col,core});
    }

    struct Line { sf::Vector3f a, b; };
    std::vector<Line> grid;

    float ext = GRID_LINES * GRID_SPACING * 0.5f;
    auto warp = [&](float x, float z) {
        float d = sqrt(x*x + z*z);
        return GRID_DEPTH + WARP_STRENGTH / (d + 45.0f);
    };

    for (int i = 0; i <= GRID_LINES; ++i) {
        float f = -ext + i * GRID_SPACING;
        for (float k = -ext; k < ext; k += 25) {
            grid.push_back({{k, warp(k,f), f}, {k+25, warp(k+25,f), f}});
            grid.push_back({{f, warp(f,k), k}, {f, warp(f,k+25), k+25}});
        }
    }

    float rotationMultiplier = 1.0f;

    while (window.isOpen()) {
        sf::Event e;
        while (window.pollEvent(e)) {
            if (e.type == sf::Event::Closed)
                window.close();

            if (e.type == sf::Event::KeyPressed) {
                if (e.key.code == sf::Keyboard::Add || e.key.code == sf::Keyboard::Equal) // Cmd + / =
                    rotationMultiplier *= 1.2f;
                else if (e.key.code == sf::Keyboard::Subtract || e.key.code == sf::Keyboard::Hyphen) // Cmd -
                    rotationMultiplier /= 1.2f;
            }
        }

        float cx = window.getSize().x / 2.0f;
        float cy = window.getSize().y / 2.0f;

        window.clear(sf::Color::Black);

        sf::VertexArray gridVA(sf::Lines);
        for (auto& l : grid) {
            gridVA.append({project(l.a.x,l.a.y,l.a.z,cx,cy), {65,65,75,70}});
            gridVA.append({project(l.b.x,l.b.y,l.b.z,cx,cy), {65,65,75,70}});
        }
        window.draw(gridVA);


        sf::VertexArray starsVA(sf::Points);
        sf::VertexArray bloomVA(sf::Points);

        for (auto& s : stars) {
            s.angle += (s.speed + N(rng) * 0.02f) * TIME_STEP * rotationMultiplier;

            float px = cos(s.angle) * s.radius;
            float pz = sin(s.angle) * s.radius;

            auto p = project(px, s.y, pz, cx, cy);

            sf::Color c = s.color;
            c.a *= exp(-abs(s.y) * 0.25f);

            starsVA.append({p, c});

            if (s.isCore) {
                sf::Color glow = c;
                glow.a = 12;
                bloomVA.append({p + sf::Vector2f(1.f, 0.f), glow});
                bloomVA.append({p + sf::Vector2f(-1.f, 0.f), glow});
                bloomVA.append({p + sf::Vector2f(0.f, 1.f), glow});
                bloomVA.append({p + sf::Vector2f(0.f, -1.f), glow});
            }
        }

        window.draw(bloomVA);
        window.draw(starsVA);
        window.display();
    }
}
