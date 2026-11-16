
#include <nds.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

/* =====================================================
   PERRO SANCHEZ: EL PODER DE ABASCAL
   Nintendo DSi - libnds
   Version 0.3 - Pixel art detallado
   ===================================================== */

#define SW 256
#define SH 192
#define WORLD_W 900
#define GROUND 165
#define PLAYER_W 22
#define PLAYER_H 38
#define MAX_COINS 12
#define MAX_ITEMS 3
#define MAX_ENEMIES 5
#define LEVELS 16

#define RGB(r,g,b) (RGB15(r,g,b) | BIT(15))

typedef enum {
    MENU,
    CREDITS,
    MAP,
    PLAY,
    PAUSE,
    GAMEOVER,
    WIN
} State;

typedef enum {
    NO_ITEM,
    HACIENDA,
    COCA,
    BLACK_CARD
} ItemType;

typedef struct {
    float x, y, vx, vy;
    int grounded;
    int facing;
    int invincible;
} Player;

typedef struct {
    int x, y, active;
} Coin;

typedef struct {
    int x, y, active;
    ItemType type;
} Item;

typedef struct {
    int x, y, active;
    int direction;
    int animation;
    int skin;
} Enemy;

typedef struct {
    int x, y, w, h;
} Platform;

typedef struct {
    int x, y;
    int health;
    int active;
    int direction;
    int timer;
} Boss;

/* =====================================================
   GLOBAL VARIABLES
   ===================================================== */

static volatile u16 *fb = (volatile u16 *)0x06000000;

static State state = MENU;

static Player player;
static Coin coins[MAX_COINS];
static Item items[MAX_ITEMS];
static Enemy enemies[MAX_ENEMIES];
static Platform platforms[6];
static Boss boss;

static int lives = 6;
static int money = 0;

static int currentLevel = 0;
static int unlockedLevel = 0;
static int selectedLevel = 0;

static int cameraX = 0;
static int frame = 0;

static int cocaTimer = 0;
static int messageTimer = 0;
static int jumpCooldown = 0;
static int damageCooldown = 0;

static char message[100] = "";

static bool bossDefeated[LEVELS];
static bool coinCollected[LEVELS][MAX_COINS];
static bool itemCollected[LEVELS][MAX_ITEMS];

static bool levelCompleted[LEVELS];

static int bossHealth = 3;
static int bossMaxHealth = 3;

static bool finalBattle = false;

/* =====================================================
   LEVEL INFORMATION
   ===================================================== */

static const char *levelNames[LEVELS] = {
    "GALICIA",
    "ASTURIAS",
    "CANTABRIA",
    "PAIS VASCO",
    "NAVARRA",
    "ARAGON",
    "CATALUNA",
    "CASTILLA Y LEON",
    "LA RIOJA",
    "MADRID",
    "EXTREMADURA",
    "CASTILLA LA MANCHA",
    "VALENCIA",
    "MURCIA",
    "ANDALUCIA",
    "MADRID - FINAL"
};

static const char *levelCities[LEVELS] = {
    "Santiago",
    "Oviedo",
    "Santander",
    "Bilbao",
    "Pamplona",
    "Zaragoza",
    "Barcelona",
    "Valladolid",
    "Logrono",
    "Madrid",
    "Merida",
    "Toledo",
    "Valencia",
    "Murcia",
    "Granada",
    "La Moncloa"
};

static const u16 skyColors[LEVELS] = {
    RGB(17,25,31), RGB(15,24,31), RGB(18,25,31), RGB(17,23,30), RGB(20,26,31),
    RGB(24,24,20), RGB(18,25,31), RGB(24,24,20), RGB(18,25,29), RGB(20,25,31),
    RGB(27,23,17), RGB(27,25,19), RGB(28,26,20), RGB(28,25,18), RGB(27,24,18), RGB(18,23,31)
};

/* =====================================================
   GRAPHICS
   ===================================================== */

static void clearScreen(u16 color) {
    int i;

    for (i = 0; i < SW * SH; i++)
        fb[i] = color;
}

static void pixel(int x, int y, u16 color) {
    if (x >= 0 && x < SW && y >= 0 && y < SH)
        fb[y * SW + x] = color;
}

static void rect(int x, int y, int w, int h, u16 color) {
    int i, j;

    for (j = 0; j < h; j++)
        for (i = 0; i < w; i++)
            pixel(x + i, y + j, color);
}

static void borderRect(int x, int y, int w, int h, u16 color) {
    rect(x, y, w, 2, color);
    rect(x, y + h - 2, w, 2, color);
    rect(x, y, 2, h, color);
    rect(x + w - 2, y, 2, h, color);
}

/* =====================================================
   PIXEL ART CHARACTERS
   ===================================================== */

/* Personajes dibujados a mano con bloques de pixel art. */
static void drawPedro(int x, int y) {
    u16 skin=RGB(24,17,12), shadow=RGB(16,10,8), hair=RGB(7,5,4);
    u16 suit=RGB(5,9,19), suitHi=RGB(9,15,28), shirt=RGB(29,29,27);
    u16 tie=RGB(25,4,5), white=RGB(31,31,31);
    /* pelo y cabeza */
    rect(x+5,y,13,5,hair); rect(x+3,y+3,17,5,hair);
    rect(x+4,y+6,16,13,skin); rect(x+4,y+15,16,3,shadow);
    rect(x+6,y+9,3,2,white); rect(x+15,y+9,3,2,white);
    pixel(x+7,y+10,RGB(2,3,5)); pixel(x+16,y+10,RGB(2,3,5));
    rect(x+10,y+13,5,2,RGB(17,7,6)); rect(x+9,y+16,7,2,RGB(18,5,5));
    /* cuello, camisa y traje */
    rect(x+9,y+18,7,4,skin); rect(x+3,y+21,18,11,suit);
    rect(x+8,y+20,8,12,shirt); rect(x+10,y+21,4,10,tie);
    rect(x+2,y+22,5,12,suitHi); rect(x+18,y+22,5,12,suitHi);
    pixel(x+6,y+25,RGB(20,22,26)); pixel(x+17,y+25,RGB(20,22,26));
    /* piernas y zapatos */
    rect(x+5,y+32,6,5,suit); rect(x+14,y+32,6,5,suit);
    rect(x+3,y+36,8,2,RGB(3,3,5)); rect(x+14,y+36,8,2,RGB(3,3,5));
}

static void drawAbascal(int x, int y) {
    u16 skin=RGB(23,16,11), shadow=RGB(13,8,7), hair=RGB(6,5,5);
    u16 beard=RGB(8,6,5), suit=RGB(8,9,14), shirt=RGB(29,29,27);
    rect(x+7,y,16,5,hair); rect(x+4,y+3,20,5,hair);
    rect(x+5,y+6,18,13,skin);
    rect(x+8,y+10,3,2,RGB(31,31,31)); rect(x+18,y+10,3,2,RGB(31,31,31));
    pixel(x+9,y+11,RGB(1,2,3)); pixel(x+19,y+11,RGB(1,2,3));
    /* barba y mandíbula */
    rect(x+6,y+15,17,6,beard); rect(x+9,y+18,11,3,shadow);
    rect(x+10,y+20,8,4,skin);
    rect(x+3,y+23,20,10,suit); rect(x+9,y+22,9,11,shirt);
    rect(x+12,y+23,3,10,RGB(4,7,13));
    rect(x+2,y+24,5,10,suit); rect(x+20,y+24,5,10,suit);
    rect(x+6,y+33,6,4,suit); rect(x+16,y+33,6,4,suit);
    rect(x+4,y+36,8,2,RGB(2,2,3)); rect(x+16,y+36,8,2,RGB(2,2,3));
}

static void drawPedroBig(int x,int y) {
    /* retrato ampliado, conservando el pixelado */
    rect(x+10,y,28,10,RGB(7,5,4)); rect(x+7,y+7,34,25,RGB(24,17,12));
    rect(x+13,y+18,6,4,RGB(31,31,31)); rect(x+30,y+18,6,4,RGB(31,31,31));
    pixel(x+16,y+20,RGB(1,2,4)); pixel(x+33,y+20,RGB(1,2,4));
    rect(x+19,y+27,12,4,RGB(17,7,6));
    rect(x+5,y+34,38,29,RGB(5,9,19)); rect(x+15,y+34,18,28,RGB(29,29,27));
    rect(x+22,y+36,5,25,RGB(25,4,5));
    rect(x+9,y+63,13,10,RGB(5,9,19)); rect(x+28,y+63,13,10,RGB(5,9,19));
}
static void drawAbascalBig(int x,int y) {
    rect(x+11,y,27,10,RGB(6,5,5)); rect(x+6,y+7,37,26,RGB(23,16,11));
    rect(x+14,y+18,6,4,RGB(31,31,31)); rect(x+31,y+18,6,4,RGB(31,31,31));
    pixel(x+17,y+20,RGB(1,2,3)); pixel(x+34,y+20,RGB(1,2,3));
    rect(x+9,y+28,32,9,RGB(8,6,5)); rect(x+16,y+35,19,5,RGB(8,6,5));
    rect(x+5,y+40,39,24,RGB(8,9,14)); rect(x+16,y+40,18,23,RGB(29,29,27));
    rect(x+22,y+42,5,21,RGB(4,7,13));
    rect(x+10,y+64,13,9,RGB(8,9,14)); rect(x+29,y+64,13,9,RGB(8,9,14));
}

static void drawPSGEnemy(int x, int y, int animation, int skinType) {
    u16 skin = skinType==0 ? RGB(10,7,5) : (skinType==1 ? RGB(19,13,9) : RGB(25,18,13));
    u16 blue=RGB(3,8,27), blueHi=RGB(7,16,40), black=RGB(2,2,3);
    /* gorra con visera y cabeza */
    rect(x+4,y,14,4,black); rect(x+7,y-2,10,3,RGB(20,16,8));
    rect(x+15,y+3,8,2,black); rect(x+5,y+4,14,11,skin);
    pixel(x+8,y+8,RGB(31,31,31)); pixel(x+15,y+8,RGB(31,31,31));
    pixel(x+9,y+8,RGB(1,2,3)); pixel(x+16,y+8,RGB(1,2,3));
    rect(x+10,y+12,6,2,RGB(12,5,5));
    /* chándal con cremallera y bandas */
    rect(x+2,y+15,20,12,blue); rect(x+3,y+16,4,10,blueHi);
    rect(x+17,y+16,4,10,blueHi); rect(x+11,y+15,2,11,RGB(31,3,5));
    if(animation%2==0){rect(x+4,y+27,6,5,blue);rect(x+14,y+27,6,3,blue);}
    else{rect(x+4,y+27,6,3,blue);rect(x+14,y+27,6,5,blue);}
    rect(x+2,y+31,9,2,black); rect(x+14,y+31,9,2,black);
}

/* Mini jefe regional */

static void drawRegionalBoss(int x, int y) {
    rect(x+4,y,18,17,RGB(24,17,12));
    rect(x+3,y,20,5,RGB(7,6,5));

    pixel(x+9,y+8,RGB(31,31,31));
    pixel(x+17,y+8,RGB(31,31,31));

    rect(x+2,y+17,22,22,RGB(10,10,18));
    rect(x+9,y+18,7,16,RGB(31,31,31));

    rect(x+4,y+39,6,7,RGB(5,5,8));
    rect(x+16,y+39,6,7,RGB(5,5,8));
}

/* =====================================================
   BACKGROUNDS
   ===================================================== */

static void drawBackground(void) {
    int i;
    clearScreen(skyColors[currentLevel]);
    /* nubes claras */
    for(i=0;i<4;i++){
        int x=(i*83-cameraX/5)%300; if(x< -35)x+=300;
        rect(x,27+(i%2)*15,28,5,RGB(31,31,31));
        rect(x+6,22+(i%2)*15,16,5,RGB(31,31,31));
    }
    rect(210,17,17,17,RGB(31,27,12)); rect(214,13,9,25,RGB(31,29,18));
    /* cordillera en tres planos */
    for(i=0;i<5;i++){
        int x=(i*76-cameraX/4)%330; if(x< -55)x+=330;
        rect(x,112,55,34,RGB(12,18,22)); rect(x+12,98,30,18,RGB(17,22,25));
        rect(x+20,99,10,7,RGB(27,29,29));
        rect(x+7,125,4,13,RGB(8,22,13)); rect(x+40,120,5,18,RGB(8,22,13));
    }
    /* Cada comunidad tiene un decorado reconocible y diferente. */
    switch(currentLevel) {
      case 0: /* Galicia: catedral y tejados de Santiago */
        rect(24,100,50,45,RGB(23,18,13)); rect(31,87,35,13,RGB(27,22,16));
        rect(39,72,8,15,RGB(28,23,17)); rect(52,68,8,19,RGB(28,23,17));
        rect(35,94,28,4,RGB(31,25,16)); break;
      case 1: /* Asturias: Picos de Europa */
        rect(0,107,90,38,RGB(8,21,13)); rect(20,89,38,28,RGB(15,22,23));
        rect(32,77,15,15,RGB(28,29,28)); rect(70,98,45,47,RGB(9,22,13)); break;
      case 2: /* Cantabria: costa y faro */
        rect(0,130,256,15,RGB(8,19,28)); rect(185,82,9,62,RGB(24,21,15));
        rect(179,76,21,7,RGB(29,26,18)); rect(183,68,13,8,RGB(31,27,14));
        rect(0,140,256,5,RGB(16,25,29)); break;
      case 3: /* País Vasco: museo moderno */
        rect(155,95,65,50,RGB(20,21,22)); rect(166,83,44,12,RGB(27,27,25));
        rect(173,76,32,7,RGB(31,29,23)); rect(164,107,48,3,RGB(31,26,16)); break;
      case 4: /* Navarra: murallas y torre */
        rect(145,102,75,43,RGB(22,18,13)); rect(155,87,14,15,RGB(26,22,16));
        rect(187,79,16,23,RGB(26,22,16)); rect(180,73,30,6,RGB(29,24,17));
        rect(151,119,64,4,RGB(29,23,15)); break;
      case 5: /* Aragón: basílica del Pilar */
        rect(139,103,80,42,RGB(24,20,15)); rect(150,88,17,15,RGB(27,23,17));
        rect(180,84,14,19,RGB(27,23,17)); rect(204,88,10,15,RGB(27,23,17));
        rect(145,99,75,4,RGB(31,26,17)); break;
      case 6: /* Cataluña: Sagrada Familia */
        rect(165,92,48,53,RGB(23,17,12)); rect(171,68,9,24,RGB(27,22,15));
        rect(185,56,8,36,RGB(27,22,15)); rect(199,72,8,20,RGB(27,22,15));
        rect(168,61,15,5,RGB(31,24,15)); rect(182,49,14,5,RGB(31,24,15));
        rect(197,65,13,5,RGB(31,24,15)); break;
      case 7: /* Castilla y León: castillo */
        rect(145,105,76,40,RGB(22,18,13)); rect(151,87,16,18,RGB(27,22,15));
        rect(176,82,18,23,RGB(27,22,15)); rect(205,91,12,14,RGB(27,22,15));
        rect(147,100,72,5,RGB(31,25,16)); break;
      case 8: /* La Rioja: viñedos y bodega */
        rect(165,105,53,40,RGB(22,18,13)); rect(175,91,33,14,RGB(27,22,15));
        for(i=0;i<5;i++) rect(18+i*22,132+(i%2)*4,12,3,RGB(15,25,10));
        rect(0,139,256,3,RGB(20,27,12)); break;
      case 9: /* Madrid: Puerta de Alcalá / ciudad */
        rect(160,91,58,54,RGB(22,17,16)); rect(169,77,40,14,RGB(27,22,18));
        rect(178,65,22,12,RGB(28,24,19)); rect(183,59,12,6,RGB(31,26,16));
        for(i=0;i<4;i++) rect(167+i*12,104,5,13,RGB(31,24,13)); break;
      case 10: /* Extremadura: teatro romano */
        rect(151,105,70,40,RGB(22,18,13));
        for(i=0;i<5;i++) rect(155+i*13,111,7,22,RGB(29,23,15));
        rect(149,103,74,5,RGB(31,25,16)); rect(160,96,50,7,RGB(26,21,15)); break;
      case 11: /* Castilla-La Mancha: molinos */
        for(i=0;i<3;i++){
          int x=35+i*68; rect(x,105,13,40,RGB(24,20,14));
          rect(x-4,101,21,5,RGB(29,24,16)); rect(x+5,85,3,18,RGB(31,29,24));
          rect(x-6,92,25,3,RGB(31,29,24)); rect(x+1,89,5,5,RGB(31,29,24));
        } break;
      case 12: /* Valencia: Ciudad de las Artes */
        rect(30,110,60,35,RGB(23,21,17)); rect(40,98,40,12,RGB(29,27,22));
        rect(48,91,24,7,RGB(31,30,27)); rect(101,119,55,26,RGB(20,23,24));
        rect(108,111,42,8,RGB(28,29,27)); break;
      case 13: /* Murcia: catedral y torre */
        rect(164,100,48,45,RGB(24,19,14)); rect(180,67,17,33,RGB(28,23,16));
        rect(176,61,25,6,RGB(31,25,16)); rect(183,53,11,8,RGB(28,23,16));
        rect(170,115,36,5,RGB(31,25,16)); break;
      case 14: /* Andalucia: Alhambra de Granada */
        rect(143,99,80,46,RGB(24,17,12)); rect(151,87,65,12,RGB(28,21,14));
        rect(159,78,13,9,RGB(29,22,15)); rect(185,73,13,14,RGB(29,22,15));
        rect(150,108,67,4,RGB(31,24,15));
        for(i=0;i<5;i++) rect(154+i*13,117,5,12,RGB(31,25,16)); break;
      default: /* La Moncloa: sede del combate final */
        rect(151,93,75,52,RGB(22,17,16)); rect(161,77,55,16,RGB(27,22,18));
        rect(172,66,34,11,RGB(28,24,19)); rect(180,59,18,7,RGB(31,26,16));
        for(i=0;i<5;i++) rect(159+i*13,105,6,11,RGB(31,24,13));
        rect(148,91,81,4,RGB(31,26,17)); break;
    }
    /* pradera con capas y flores */
    rect(0,GROUND,SW,27,RGB(8,22,10)); rect(0,GROUND,SW,3,RGB(20,29,12));
    rect(0,GROUND+5,SW,22,RGB(10,19,9));
    for(i=0;i<22;i++){
        int x=(i*17-cameraX/2)%270; if(x<0)x+=270;
        rect(x,150+(i%3)*4,3,7,RGB(7,26,10));
        if(i%4==0) pixel(x+4,153,RGB(31,22,13));
    }
}

/* =====================================================
   MAP
   ===================================================== */

static void fillPolygon(const int *px,const int *py,int n,u16 color) {
    int y,x,i;
    for(y=0;y<SH;y++){
        int nodes[32],count=0;
        for(i=0;i<n;i++){
            int j=(i+1)%n;
            if((py[i]<y && py[j]>=y)||(py[j]<y && py[i]>=y))
                nodes[count++]=px[i]+(y-py[i])*(px[j]-px[i])/(py[j]-py[i]);
        }
        for(i=0;i<count-1;i++){
            int k; for(k=i+1;k<count;k++) if(nodes[k]<nodes[i]){int t=nodes[i];nodes[i]=nodes[k];nodes[k]=t;}
        }
        for(i=0;i+1<count;i+=2) for(x=nodes[i];x<=nodes[i+1];x++) pixel(x,y,color);
    }
}
static void drawMap(void) {
    /* silueta reconocible de la España peninsular, no un bloque rectangular */
    static const int sx[]={43,54,62,75,88,105,122,143,163,184,204,217,211,199,189,176,162,151,137,123,109,94,80,66,54};
    static const int sy[]={63,49,44,48,42,46,42,48,46,53,61,76,91,102,115,126,136,145,147,140,143,132,124,108,87};
    static const int mx[]={49,57,65,78,92,109,128,146,166,185,203,215,205,192,180,165,151,135,118,101,84,68,55};
    static const int my[]={61,50,46,50,45,49,46,51,50,57,64,77,90,104,118,130,139,144,142,135,126,107,85};
    int i;
    clearScreen(RGB(12,22,30));
    /* Portugal y marcos de costa */
    rect(18,50,5,92,RGB(9,17,23));
    fillPolygon(sx,sy,25,RGB(8,14,17));
    fillPolygon(mx,my,23,RGB(18,25,13));
    /* regiones sugeridas por fronteras internas */
    for(i=0;i<7;i++){
        int x=75+i*17; rect(x,62+(i%3)*10,1,48,RGB(23,29,17));
    }
    rect(198,130,5,3,RGB(18,25,13)); /* Baleares */
    rect(205,137,3,2,RGB(18,25,13));
    /* frontera costera pixelada */
    for(i=0;i<25;i++) pixel(sx[i],sy[i],RGB(29,29,20));
    /* puntos colocados aproximadamente en su geografía */
    {
        static const int px[LEVELS]={52,87,108,139,153,170,190,111,139,137,77,113,205,190,154,136};
        static const int py[LEVELS]={64,57,63,68,78,91,111,99,87,111,122,121,119,136,140,112};
        for(i=0;i<LEVELS;i++){
            u16 c=(i==selectedLevel)?RGB(31,24,5):(i<=unlockedLevel?RGB(25,12,4):RGB(8,10,9));
            rect(px[i]-4,py[i]-4,9,9,c);
            rect(px[i]-2,py[i]-2,5,5,(i<=unlockedLevel)?RGB(31,31,23):RGB(15,16,14));
            if(i==15) rect(px[i]-5,py[i]-5,11,11,RGB(31,5,4));
            if(i==selectedLevel) borderRect(px[i]-6,py[i]-6,13,13,RGB(31,28,9));
        }
    }
    /* título y panel decorativo */
    rect(8,8,240,20,RGB(5,12,19)); borderRect(8,8,240,20,RGB(29,27,17));
    rect(14,14,228,2,RGB(22,28,18));
}

/* =====================================================
   INITIALIZE LEVEL
   ===================================================== */

static void initLevel(void) {
    int i;

    player.x = 15;
    player.y = GROUND-PLAYER_H;
    player.vx = 0;
    player.vy = 0;
    player.grounded = 0;
    player.facing = 1;
    player.invincible = 0;

    cameraX = 0;

    cocaTimer = 0;
    jumpCooldown = 0;
    damageCooldown = 0;

    bossMaxHealth = (currentLevel == 15) ? 8 : 3;
    bossHealth = bossDefeated[currentLevel] ? 0 : bossMaxHealth;

    boss.x = 720;
    boss.y = GROUND-46;
    boss.health = bossHealth;
    boss.active = !bossDefeated[currentLevel];
    boss.direction = -1;
    boss.timer = 0;

    finalBattle = (currentLevel == 15);

    for (i=0; i<6; i++) {
        platforms[i].x = 125+i*125;
        platforms[i].y = 125-(i%2)*25;
        platforms[i].w = 45;
        platforms[i].h = 8;
    }

    for (i=0; i<MAX_COINS; i++) {
        coins[i].x = 100+i*65;
        coins[i].y = 105-(i%3)*12;
        coins[i].active = !coinCollected[currentLevel][i];
    }

    for (i=0; i<MAX_ITEMS; i++) {
        items[i].x = 220+i*240;
        items[i].y = 110;
        items[i].active = !itemCollected[currentLevel][i];

        if (i==0)
            items[i].type = HACIENDA;
        else if (i==1)
            items[i].type = COCA;
        else
            items[i].type = BLACK_CARD;
    }

    /* Hacienda muy poco frecuente */
    if (currentLevel != 3 &&
        currentLevel != 8 &&
        currentLevel != 13) {
        items[0].active = 0;
    }

    for (i=0; i<MAX_ENEMIES; i++) {
        enemies[i].x = 180+i*115;
        enemies[i].y = GROUND-33;
        enemies[i].active = 1;
        enemies[i].direction = -1;
        enemies[i].animation = 0;
        enemies[i].skin = i%3;
    }

    if (bossDefeated[currentLevel])
        boss.active = 0;
}

/* =====================================================
   MESSAGE
   ===================================================== */

static void showMessage(const char *text) {
    strncpy(message,text,sizeof(message)-1);
    message[sizeof(message)-1]='\0';
    messageTimer=150;
}

/* =====================================================
   LIFE SYSTEM
   ===================================================== */

static void loseLife(void) {
    lives--;

    if (lives <= 0) {
        lives=6;
        money=0;

        memset(coinCollected,0,sizeof(coinCollected));
        memset(itemCollected,0,sizeof(itemCollected));
        memset(bossDefeated,0,sizeof(bossDefeated));
        memset(levelCompleted,0,sizeof(levelCompleted));

        currentLevel=0;
        unlockedLevel=0;
        selectedLevel=0;

        state=GAMEOVER;
    } else {
        initLevel();
        showMessage("Has perdido una vida!");
    }
}

/* =====================================================
   ITEMS
   ===================================================== */

static void collectItem(int i) {
    if (!items[i].active)
        return;

    items[i].active=0;
    itemCollected[currentLevel][i]=true;

    if (items[i].type == HACIENDA) {
        money /= 2;
        showMessage("HACIENDA! Pierdes el 50% de monedas.");
    }

    if (items[i].type == COCA) {
        cocaTimer=360;
        showMessage("COCA! Velocidad aumentada.");
    }

    if (items[i].type == BLACK_CARD) {
        money+=100;
        showMessage("TARJETA BLACK! +100 MONEDAS.");
    }
}

/* =====================================================
   COLLISION
   ===================================================== */

static bool collision(int x1,int y1,int w1,int h1,
                      int x2,int y2,int w2,int h2) {
    return x1 < x2+w2 &&
           x1+w1 > x2 &&
           y1 < y2+h2 &&
           y1+h1 > y2;
}

/* =====================================================
   PLAYER UPDATE
   ===================================================== */

static void updatePlayer(u32 held,u32 pressed) {
    float speed = cocaTimer>0 ? 3.5f : 2.0f;

    player.vx=0;

    /* Cruceta: izquierda y derecha para moverse */
    if (held & KEY_RIGHT) {
        player.vx=speed;
        player.facing=1;
    }
    if (held & KEY_LEFT) {
        player.vx=-speed;
        player.facing=-1;
    }

    /* B = saltar */
    if ((pressed & KEY_B) && player.grounded) {
        player.vy=-6.0f;
        player.grounded=0;
    }

    player.x+=player.vx;

    if (player.x<0)
        player.x=0;

    if (player.x>WORLD_W-PLAYER_W)
        player.x=WORLD_W-PLAYER_W;

    /* Gravedad */
    player.vy+=0.25f;

    if (player.vy>6.0f)
        player.vy=6.0f;

    player.y+=player.vy;

    player.grounded=0;

    /* Suelo */
    if (player.y+PLAYER_H>=GROUND) {
        player.y=GROUND-PLAYER_H;
        player.vy=0;
        player.grounded=1;
    }

    /* Plataformas */
    {
        int i;

        for (i=0;i<6;i++) {
            Platform p=platforms[i];

            if (player.vy>=0 &&
                player.x+PLAYER_W>p.x &&
                player.x<p.x+p.w &&
                player.y+PLAYER_H>=p.y &&
                player.y+PLAYER_H<=p.y+12) {

                player.y=p.y-PLAYER_H;
                player.vy=0;
                player.grounded=1;
            }
        }
    }

    /* Caida */
    if (player.y>SH+30)
        loseLife();

    if (player.invincible>0)
        player.invincible--;

    if (damageCooldown>0)
        damageCooldown--;

    if (jumpCooldown>0)
        jumpCooldown--;

    if (cocaTimer>0)
        cocaTimer--;
}

/* =====================================================
   COLLECT COINS
   ===================================================== */

static void updateCoins(void) {
    int i;

    for (i=0;i<MAX_COINS;i++) {
        if (!coins[i].active)
            continue;

        if (collision((int)player.x,(int)player.y,
                      PLAYER_W,PLAYER_H,
                      coins[i].x,coins[i].y,10,10)) {

            coins[i].active=0;
            coinCollected[currentLevel][i]=true;
            money++;
        }
    }
}

/* =====================================================
   COLLECT SPECIAL ITEMS
   ===================================================== */

static void updateItems(void) {
    int i;

    for (i=0;i<MAX_ITEMS;i++) {
        if (!items[i].active)
            continue;

        if (collision((int)player.x,(int)player.y,
                      PLAYER_W,PLAYER_H,
                      items[i].x,items[i].y,16,16)) {
            collectItem(i);
        }
    }
}

/* =====================================================
   ENEMIES
   ===================================================== */

static void updateEnemies(void) {
    int i;

    for (i=0;i<MAX_ENEMIES;i++) {
        if (!enemies[i].active)
            continue;

        enemies[i].x+=enemies[i].direction;
        enemies[i].animation++;

        if (enemies[i].x<30)
            enemies[i].direction=1;

        if (enemies[i].x>WORLD_W-40)
            enemies[i].direction=-1;

        if (collision((int)player.x,(int)player.y,
                      PLAYER_W,PLAYER_H,
                      enemies[i].x,enemies[i].y,18,27)) {

            if (player.vy>0 &&
                player.y+PLAYER_H<enemies[i].y+15) {

                enemies[i].active=0;
                player.vy=-4.0f;
                money+=5;

            } else if (damageCooldown==0 &&
                       player.invincible==0) {

                damageCooldown=60;
                player.invincible=60;
                loseLife();
                return;
            }
        }
    }
}

/* =====================================================
   MINI BOSSES AND FINAL BOSS
   ===================================================== */

static void updateBoss(void) {
    if (!boss.active)
        return;

    boss.timer++;

    if (boss.timer%60==0)
        boss.direction=-boss.direction;

    boss.x+=boss.direction;

    if (boss.x<620)
        boss.direction=1;

    if (boss.x>760)
        boss.direction=-1;

    if (collision((int)player.x,(int)player.y,
                  PLAYER_W,PLAYER_H,
                  boss.x,boss.y,30,46)) {

        if (player.vy>1.0f &&
            player.y+PLAYER_H<boss.y+15) {

            boss.health--;
            bossHealth=boss.health;
            player.vy=-5.0f;

            if (boss.health<=0) {
                boss.active=0;
                bossDefeated[currentLevel]=true;
                money+=50;
                showMessage("MINI JEFE DERROTADO! +50");
            }

        } else if (damageCooldown==0 &&
                   player.invincible==0) {

            damageCooldown=60;
            player.invincible=60;
            loseLife();
        }
    }
}

/* =====================================================
   LEVEL PROGRESSION
   ===================================================== */

static void updateProgress(void) {
    if (!boss.active && player.x>WORLD_W-70) {

        levelCompleted[currentLevel]=true;

        if (currentLevel<15) {
            if (currentLevel>=unlockedLevel)
                unlockedLevel=currentLevel+1;

            selectedLevel=currentLevel+1;
            state=MAP;
        } else {
            state=WIN;
        }
    }
}

/* =====================================================
   CAMERA
   ===================================================== */

static void updateCamera(void) {
    cameraX=(int)player.x-100;

    if (cameraX<0)
        cameraX=0;

    if (cameraX>WORLD_W-SW)
        cameraX=WORLD_W-SW;
}

/* =====================================================
   GAME RENDERING
   ===================================================== */

static void drawGame(void) {
    int i;
    int sx;

    drawBackground();

    /* Platforms */
    for (i=0;i<6;i++) {
        sx=platforms[i].x-cameraX;
        rect(sx,platforms[i].y,platforms[i].w,
             platforms[i].h,RGB(10,20,10));
        rect(sx,platforms[i].y,platforms[i].w,
             2,RGB(20,28,12));
    }

    /* Coins */
    for (i=0;i<MAX_COINS;i++) {
        if (coins[i].active) {
            sx=coins[i].x-cameraX;
            rect(sx+2,coins[i].y,6,10,RGB(31,23,2));
            rect(sx,coins[i].y+2,10,6,RGB(31,27,5));
        }
    }

    /* Items */
    for (i=0;i<MAX_ITEMS;i++) {
        if (!items[i].active)
            continue;

        sx=items[i].x-cameraX;

        if (items[i].type==HACIENDA) {
            rect(sx,items[i].y,16,16,RGB(25,25,25));
            rect(sx+3,items[i].y+3,10,10,RGB(5,5,5));
            pixel(sx+7,items[i].y+6,RGB(31,5,5));
        }

        if (items[i].type==COCA) {
            rect(sx+3,items[i].y,10,16,RGB(5,25,5));
            rect(sx+5,items[i].y+3,6,10,RGB(31,31,31));
        }

        if (items[i].type==BLACK_CARD) {
            rect(sx,items[i].y+3,18,11,RGB(2,2,3));
            rect(sx+3,items[i].y+6,5,4,RGB(31,22,5));
        }
    }

    /* Enemies */
    for (i=0;i<MAX_ENEMIES;i++) {
        if (enemies[i].active) {
            sx=enemies[i].x-cameraX;
            drawPSGEnemy(sx,enemies[i].y,
                         enemies[i].animation,
                         enemies[i].skin);
        }
    }

    /* Mini boss */
    if (boss.active) {
        sx=boss.x-cameraX;

        if (currentLevel==15)
            drawAbascal(sx,boss.y);
        else
            drawRegionalBoss(sx,boss.y);
    }

    /* Player */
    if (player.invincible==0 || frame%8<4)
        drawPedro((int)player.x-cameraX,(int)player.y);

    /* Exit */
    if (!boss.active)
        rect(WORLD_W-45-cameraX,GROUND-35,
             25,35,RGB(20,15,5));

    /* Boss health bar */
    if (boss.active) {
        rect(75,8,110,8,RGB(10,3,3));
        rect(77,10,
             (boss.health*106)/bossMaxHealth,4,
             RGB(28,3,3));
    }

    /* Message */
    if (messageTimer>0) {
        borderRect(15,42,226,30,RGB(31,25,5));
        rect(17,44,222,26,RGB(3,5,12));
    }
}

/* =====================================================
   MENU RENDERING
   ===================================================== */

static void drawMenu(void) {
    int i;
    clearScreen(RGB(18,25,31));
    /* cielo, nubes y sol */
    rect(0,0,256,112,RGB(17,25,31)); rect(199,15,25,25,RGB(31,27,12));
    for(i=0;i<4;i++){int x=15+i*68;rect(x,27,27,5,RGB(31,31,31));rect(x+6,22,15,5,RGB(31,31,31));}
    /* colinas */
    rect(0,104,256,24,RGB(12,22,13)); rect(0,117,256,14,RGB(8,20,10));
    rect(0,130,256,62,RGB(10,24,12));
    for(i=0;i<13;i++){int x=i*22;rect(x,119+(i%3)*5,3,14,RGB(6,18,9));rect(x+5,122+(i%2)*4,2,10,RGB(6,18,9));}
    /* título como placa de videojuego */
    rect(28,8,200,27,RGB(7,16,26)); borderRect(28,8,200,27,RGB(31,25,8));
    rect(34,14,188,3,RGB(28,8,6)); rect(34,26,188,3,RGB(28,8,6));
    /* personajes grandes y reconocibles */
    drawPedroBig(30,54); drawAbascalBig(181,54);
    /* botones */
    rect(78,139,100,19,RGB(9,25,13)); borderRect(78,139,100,19,RGB(31,27,8));
    rect(78,163,100,19,RGB(8,15,27)); borderRect(78,163,100,19,RGB(25,29,31));
    rect(86,145,84,2,RGB(20,31,18)); rect(86,169,84,2,RGB(17,23,31));
}

/* =====================================================
   CREDITS
   ===================================================== */

static void drawCredits(void) {
    int i;

    clearScreen(RGB(15,23,31));
    rect(0,135,256,57,RGB(8,22,13));
    rect(0,130,256,5,RGB(20,29,14));

    /* Instagram pixel art */
    borderRect(100,35,55,55,RGB(31,20,20));
    borderRect(107,42,41,41,RGB(31,31,31));
    borderRect(116,51,23,23,RGB(31,31,31));

    rect(139,45,5,5,RGB(31,20,20));

    /* Decoracion */
    for (i=0;i<5;i++)
        rect(25+i*45,120,10,10,RGB(15,5,20));
}

/* =====================================================
   GAME OVER
   ===================================================== */

static void drawGameOver(void) {
    clearScreen(RGB(15,2,3));
    rect(45,65,166,60,RGB(5,2,2));
    borderRect(45,65,166,60,RGB(31,5,5));
}

/* =====================================================
   VICTORY
   ===================================================== */

static void drawVictory(void) {
    clearScreen(RGB(5,18,8));
    drawPedro(115,75);
    rect(70,120,116,4,RGB(31,25,3));
    rect(85,135,86,4,RGB(31,31,31));
}

/* =====================================================
   PAUSE
   ===================================================== */

static void drawPause(void) {
    drawGame();
    rect(65,65,126,60,RGB(3,4,12));
    borderRect(65,65,126,60,RGB(31,25,5));
}

/* =====================================================
   BOTTOM SCREEN CONSOLE
   ===================================================== */

static void updateConsole(void) {
    consoleClear();

    iprintf("\n  PERRO SANCHEZ\n");
    iprintf("  EL PODER DE ABASCAL\n");
    iprintf("--------------------------\n\n");

    if (state==MENU) {
        iprintf("  A - JUGAR\n");
        iprintf("  Y - CREDITOS\n");
        iprintf("\n  Flechas: moverse | B: saltar");
    }

    else if (state==CREDITS) {
        iprintf("  INSTAGRAM\n\n");
        iprintf("  juaan.pd\n\n");
        iprintf("  B - VOLVER");
    }

    else if (state==MAP) {
        iprintf("  MAPA DE ESPANA\n\n");
        iprintf("  NIVEL: %s\n",levelNames[selectedLevel]);
        iprintf("  CIUDAD: %s\n",levelCities[selectedLevel]);
        iprintf("\n  IZQ/DCHA - ELEGIR\n");
        iprintf("  A - ENTRAR\n");
        iprintf("  B - VOLVER");
    }

    else if (state==PLAY || state==PAUSE) {
        iprintf("  NIVEL %d / 16\n",currentLevel+1);
        iprintf("  %s\n",levelNames[currentLevel]);
        iprintf("  CIUDAD: %s\n",levelCities[currentLevel]);

        iprintf("\n  VIDAS: %d\n",lives);
        iprintf("  MONEDAS: %d\n",money);
        iprintf("  <- -> MOVER | B SALTAR\n");

        if (cocaTimer>0)
            iprintf("  COCA ACTIVA!\n");

        if (boss.active)
            iprintf("  JEFE: %d / %d\n",
                    boss.health,bossMaxHealth);

        if (messageTimer>0)
            iprintf("\n  %s\n",message);

        if (state==PAUSE)
            iprintf("\n  PAUSA - START CONTINUAR");
    }

    else if (state==GAMEOVER) {
        iprintf("  GAME OVER\n");
        iprintf("\n  HAS PERDIDO TODAS LAS VIDAS\n");
        iprintf("\n  A - MENU");
    }

    else if (state==WIN) {
        iprintf("  ENHORABUENA!\n");
        iprintf("\n  HAS DERROTADO A ABASCAL\n");
        iprintf("\n  A - VOLVER AL MENU");
    }
}

/* =====================================================
   MAIN
   ===================================================== */

int main(void) {
    u32 pressed;
    u32 held;

    /* Pantalla superior framebuffer */
    videoSetMode(MODE_FB0);
    vramSetBankA(VRAM_A_LCD);
    fb = (volatile u16 *)VRAM_A;

    /* Pantalla inferior consola */
    videoSetModeSub(MODE_0_2D);
    vramSetBankC(VRAM_C_SUB_BG);

    consoleInit(NULL,0,
                BgType_Text4bpp,
                BgSize_T_256x256,
                31,0,false,true);

    consoleClear();

    memset(bossDefeated,0,sizeof(bossDefeated));
    memset(coinCollected,0,sizeof(coinCollected));
    memset(itemCollected,0,sizeof(itemCollected));
    memset(levelCompleted,0,sizeof(levelCompleted));

    lives=6;
    money=0;
    currentLevel=0;
    unlockedLevel=0;
    selectedLevel=0;

    initLevel();

    while (1) {
        scanKeys();

        pressed=keysDown();
        held=keysHeld();

        frame++;

        if (messageTimer>0)
            messageTimer--;

        switch (state) {

            case MENU:

                drawMenu();

                if (pressed & KEY_A) {
                    state=MAP;
                }

                if (pressed & KEY_Y) {
                    state=CREDITS;
                }

                if (pressed & KEY_START) {
                    state=MAP;
                }

                break;

            case CREDITS:

                drawCredits();

                if (pressed & KEY_B)
                    state=MENU;

                break;

            case MAP:

                drawMap();

                if (pressed & KEY_LEFT) {
                    selectedLevel--;
                    if (selectedLevel<0)
                        selectedLevel=0;
                }

                if (pressed & KEY_RIGHT) {
                    selectedLevel++;
                    if (selectedLevel>=LEVELS)
                        selectedLevel=LEVELS-1;
                }

                if (pressed & KEY_UP) {
                    selectedLevel-=5;
                    if (selectedLevel<0)
                        selectedLevel=0;
                }

                if (pressed & KEY_DOWN) {
                    selectedLevel+=5;
                    if (selectedLevel>=LEVELS)
                        selectedLevel=LEVELS-1;
                }

                if (pressed & KEY_A) {
                    if (selectedLevel<=unlockedLevel) {
                        currentLevel=selectedLevel;
                        initLevel();
                        state=PLAY;
                    }
                }

                if (pressed & KEY_B)
                    state=MENU;

                break;

            case PLAY:

                if (pressed & KEY_START) {
                    state=PAUSE;
                    break;
                }

                updatePlayer(held,pressed);
                updateCoins();
                updateItems();
                updateEnemies();
                updateBoss();
                updateProgress();
                updateCamera();

                drawGame();

                break;

            case PAUSE:

                drawPause();

                if (pressed & KEY_START)
                    state=PLAY;

                if (pressed & KEY_B)
                    state=MAP;

                break;

            case GAMEOVER:

                drawGameOver();

                if (pressed & KEY_A)
                    state=MENU;

                break;

            case WIN:

                drawVictory();

                if (pressed & KEY_A)
                    state=MENU;

                break;
        }

        /* Actualizar informacion inferior */
        if (frame%10==0)
            updateConsole();

        swiWaitForVBlank();
    }

    return 0;
}

