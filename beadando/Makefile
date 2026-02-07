all:
	gcc -Iinclude/ src/main.c src/app.c src/camera.c src/input.c src/math3d.c src/renderer.c src/ui.c -lmingw32 -lSDL2main -lSDL2 -lopengl32 -lglu32 -lm -o solar.exe -Wall -Wextra -Wpedantic

linux:
	gcc -Iinclude/ src/main.c src/app.c src/camera.c src/input.c src/math3d.c src/renderer.c src/ui.c -lSDL2 -lGL -lGLU -lm -o solar -Wall -Wextra -Wpedantic
