#pragma once

extern const char* TARGET_PATH;

class Game {
public:
    Game();
    ~Game();

    bool isInit() const;
    int run();

private:
    bool m_isInit;
};