#include <cstdlib>
#include <application/game.hpp>


int main(int, char*[]) {
    Game game = Game();
    bool success;

    if(game.isInit()) {
        success = game.run();
    }

    system("pause");
    return success;
}
