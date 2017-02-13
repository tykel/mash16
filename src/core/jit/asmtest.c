#include <string.h>
#include <stdio.h>

struct small {
    int* v1;
    int v2;
};

void function(struct small *s)
{
    memset(s->v1, 0, 320*240*4);
}

int main(int argc, char *argv[])
{
    struct small s;
    //function(&s);

    int disp1 = (void *)&s.v2 - (void *)&s.v1;
    int disp2 = (void *)&s.v1 - (void *)&s.v2;
    printf("&v2 - &v1: %d\n&v1 - &v2: %d\n", disp1, disp2);
    return 0;
}
