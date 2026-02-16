#include <stdio.h>
#include <stdbool.h>


typedef struct{
    double length;
    double width;
    double height;
}Cuboid;

void set_size(Cuboid *c, double l, double w, double h){
    if(l>0 && w>0 && h>0){
        c->length = l;
        c->width = w;
        c->height = h;
    }
    else{
        printf("Hiba: Az oldalak hossza pizitiv szam kell legyen");
    }
}

double calc_volume(Cuboid c){
    return c.length * c.width * c.height;
}

double calc_surface(Cuboid c){
    return 2 * (c.length * c.width + c.length * c.height + c.width * c.height);
}

bool has_square_face(Cuboid c){
    return (c.length == c.width || c.length == c.height || c.width == c.height);
}

int main(){

    Cuboid myCuboid;

    set_size(&myCuboid, 3.0, 4.0, 5.0);
    printf("Terfogat: %.2f\n", calc_volume(myCuboid));
    printf("Felszin: %.2f\n", calc_surface(myCuboid));

    if(has_square_face(myCuboid)){
        printf("Van negyzet alaku alapja\n");
    }
    else{
        printf("Nincs negyzet alaku alapja\n");
    }

    return 0;
}