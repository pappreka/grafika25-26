all:
	gcc -Iinclude/ src/main.c src/app.c src/camera.c src/input.c src/math3d.c src/renderer.c src/ui.c src/mesh.c src/texture.c src/stb_image_impl.c -lmingw32 -lSDL2main -lSDL2 -lopengl32 -lglu32 -lm -o solar.exe -Wall -Wextra -Wpedantic

linux:
	gcc -Iinclude/ src/main.c src/app.c src/camera.c src/input.c src/math3d.c src/renderer.c src/ui.c src/mesh.c src/texture.c src/stb_image_impl.c -lSDL2 -lGL -lGLU -lm -o solar -Wall -Wextra -Wpedantic

clean:
	del /Q solar.exe 2>nul || exit 0
