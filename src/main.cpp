// SDL.h сохраняет платформенный SDL_main для Windows и Android.
#include <SDL.h>
#include "platform/application.h"

int main(int argc, char** argv)
{
    return kolek::runApplication(argc, argv);
}
