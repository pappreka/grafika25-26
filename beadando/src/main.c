#include "app.h"
#include <stdio.h>

// A program belépési pontja
int main(int argc, char *argv[]){
    App app;

    // Alkalmazás inicializálása
    if(!app_init(&app, "Solar System Explorer", 1280, 720)){
        fprintf(stderr, "Failed to initialize app\n");
        return 1;
    }

    // Fő programciklus futtatása
    app_run(&app);

    // Erőforrások felszabadítása
    app_shutdown(&app);

    return 0;
}