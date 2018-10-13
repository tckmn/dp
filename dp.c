#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <png.h>

#define REALSIZE 512
#define POINTS 25
#define RARITY 10
#define MIN -1
#define MAX 5
#define BLUR 2
#define PERTURB 25
#define SIZE (REALSIZE+BLUR*2)
static unsigned char img[SIZE][SIZE][3] = {0};
static unsigned char vor[SIZE][SIZE][3];
static unsigned char real[REALSIZE][REALSIZE][3];
static double points[3][POINTS][2];

#define max(a,b) ((a)>(b)?(a):(b))
#define min(a,b) ((a)<(b)?(a):(b))
#define ri(a,b) (a+((double)rand()/RAND_MAX)*(b-a+1))
int clip(int a) { return a < 0 ? 0 : a > 255 ? 255 : a; }

void go(int y, int x) {
    if (img[y][x][0] || img[y][x][1] || img[y][x][2]) return;
    if (rand() % RARITY) {
        int yy = y + ri(MIN, MAX), xx = x + ri(MIN, MAX);
        if (yy < 0 || yy >= SIZE || xx < 0 || xx >= SIZE) goto skip;
        go(yy, xx);
        img[y][x][0] = clip(img[yy][xx][0] + (int)ri(-PERTURB, PERTURB));
        img[y][x][1] = clip(img[yy][xx][1] + (int)ri(-PERTURB, PERTURB));
        img[y][x][2] = clip(img[yy][xx][2] + (int)ri(-PERTURB, PERTURB));
    } else {
skip:
        img[y][x][0] = vor[y][x][0];
        img[y][x][1] = vor[y][x][1];
        img[y][x][2] = vor[y][x][2];
    }
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "usage: %s [seed]\n", argv[0]);
        return 1;
    }
    srand(atoi(argv[1]));

    // generate voronoi points
    for (int i = 0; i < POINTS; ++i) {
        points[0][i][0] = (double)rand() / RAND_MAX;
        points[0][i][1] = (double)rand() / RAND_MAX;
        points[1][i][0] = (double)rand() / RAND_MAX;
        points[1][i][1] = (double)rand() / RAND_MAX;
        points[2][i][0] = (double)rand() / RAND_MAX;
        points[2][i][1] = (double)rand() / RAND_MAX;
    }

    // generate voronoi diagram
    for (int y = 0; y < SIZE; ++y) {
        for (int x = 0; x < SIZE; ++x) {
            for (int ch = 0; ch < 3; ++ch) {
                int closest;
                double dist = SIZE, tmp;
                for (int i = 0; i < POINTS; ++i) {
                    if ((tmp = pow((x+0.5)/SIZE - points[ch][i][0], 2) +
                               pow((y+0.5)/SIZE - points[ch][i][1], 2)) < dist) {
                        closest = i;
                        dist = tmp;
                    }
                }
                vor[x][y][ch] = ((closest+1) * 255) / POINTS;
            }
        }
    }

    // do the thing
    for (int y = 0; y < SIZE; ++y) for (int x = 0; x < SIZE; ++x) go(y, x);

    // blurring
    double ker[BLUR+1][BLUR+1];
    double kersum = 0;
    for (int i = 0; i <= BLUR; ++i) {
        for (int j = 0; j <= BLUR; ++j) {
            ker[i][j] = exp(-((i*i+j*j)/(BLUR*BLUR/2.0)));
            kersum += ker[i][j];
            if (i) kersum += ker[i][j];
            if (j) kersum += ker[i][j];
            if (i && j) kersum += ker[i][j];
        }
    }
    for (int y = BLUR; y < SIZE-BLUR; ++y) {
        for (int x = BLUR; x < SIZE-BLUR; ++x) {
            double sum[3] = {0};
            for (int by = y - BLUR; by <= y + BLUR; ++by) {
                for (int bx = x - BLUR; bx <= x + BLUR; ++bx) {
                    double mult = ker[abs(y-by)][abs(x-bx)]/kersum;
                    sum[0] += mult*img[by][bx][0];
                    sum[1] += mult*img[by][bx][1];
                    sum[2] += mult*img[by][bx][2];
                }
            }
            real[y-BLUR][x-BLUR][0] = sum[0];
            real[y-BLUR][x-BLUR][1] = sum[1];
            real[y-BLUR][x-BLUR][2] = sum[2];
        }
    }

    // write to out.png
    png_image out = {0};
    out.version = PNG_IMAGE_VERSION;
    out.width = REALSIZE;
    out.height = REALSIZE;
    out.format = PNG_FORMAT_RGB;
    png_image_write_to_file(&out, "out.png", 0, real, 0, 0);

    return 0;
}
