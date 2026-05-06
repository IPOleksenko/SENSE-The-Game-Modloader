#include <cstdlib>
#include <application/game.hpp>


int main(int, char*[]) {
    Game game = Game();

    if(game.isInit()) {
        game.run();
    }

    system("pause");
    return EXIT_SUCCESS;
}
