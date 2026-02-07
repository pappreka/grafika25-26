#include "app.h"
#include <stdio.h>

int main(int argc, char *argv[]){
    App app;
    if(!app_init(&app, "Solar System Explorer", 1280, 720)){
        fprintf(stderr, "Failed to initialize app\n");
        return 1;
    }

    app_run(&app);
    app_shutdown(&app);
    return 0;
}