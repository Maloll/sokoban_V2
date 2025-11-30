#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>

#define TAILLE 12
#define NB_DEPLACEMENTS 500

// definition des touches
#define HAUT 'z'
#define BAS 's'
#define GAUCHE 'q'
#define DROITE 'd'
#define RECOMMENCER 'r'
#define ABANDON 'x'
#define PLUS '+'
#define MOINS '-'
#define UNDO 'u'

typedef char t_Plateau[TAILLE][TAILLE];
typedef char t_tabDeplacement[NB_DEPLACEMENTS];

// definition des char a enregistrer / afficher
const char SOKOBAN[1] = "@";
const char CAISSES[1] = "$";
const char CIBLES[1] = ".";
const char MURS[1] = "#";
const char VIDE[1] = " ";
const char CAISSES_SUR_CIBLES[1] = "*";
const char SOKOBAN_SUR_CIBLE[1] = "+";
const char SOK_GAUCHE = 'g';
const char CAISSE_GAUCHE = 'G';
const char SOK_DROITE = 'd';
const char CAISSE_DROITE = 'D';
const char SOK_HAUT = 'h';
const char CAISSE_HAUT = 'H';
const char SOK_BAS = 'b';
const char CAISSE_BAS = 'B';

// prototypes de toutes les fonctions / procedures
void lecture_niveau(char niveau[]);
int kbhit();
void charger_partie(t_Plateau plateau, char fichier[]);
void enregistrer_partie(t_Plateau plateau, char fichier[]);
void afficher_plateau(t_Plateau plateau, t_Plateau niveau, int zoom);
void affiche_entete(char niveau[], int compteur);
void lecture_touches(char *Adr_touche);
void deplacer(char touche, t_Plateau plateau, int x, int y, int *adrCompteur, t_tabDeplacement deplacements);
void detection_sokoban(t_Plateau plateau, int *AdrX, int *AdrY);
bool gagne(t_Plateau plateau, t_Plateau niveau);
void zoomer(char touche, int *zoom);
void undo(t_Plateau plateau,t_tabDeplacement deplacements, int *adrCompteur, char touche, int x, int y);
void enregistrer_deplacements(t_tabDeplacement t, int nb, char fic[]);


int main(){
    //declaration des variables
    bool recommencer = true, victoire = false;
    t_Plateau plateau, niveau;
    char nomNiveau[30];
    char enregistrer = 'N', enregistrerDep = 'N';
    int zoom = 1, compteur;
    int sokobanX, sokobanY;
    t_tabDeplacement deplacements;
    char touche, verifRecommencer;
    lecture_niveau(nomNiveau);
    charger_partie(niveau, nomNiveau);

    while (recommencer && victoire == false){ // jouer les parties en boucle tant que l'utilisateur n'a pas abandonner / gagner
        // remise a 0
        touche = '\0';
        verifRecommencer = '\0';
        compteur = 0;        

        // initialisation
        charger_partie(plateau, nomNiveau);
        system("clear");
        affiche_entete(nomNiveau, compteur);
        afficher_plateau(plateau, niveau, zoom);

        while (touche != ABANDON && victoire == false){ // boucle d'affichage + deplacements + undo, tant que pas de victorie ou d'abandon
            usleep(60000); // delay pour ne pas prendre trop de ressources
            victoire = gagne(plateau, niveau);
            if (kbhit()){ // si touche appuyé
                lecture_touches(&touche);
                detection_sokoban(plateau, &sokobanX, &sokobanY);
                deplacer(touche, plateau, sokobanX, sokobanY, &compteur,deplacements);
                undo(plateau,deplacements,&compteur,touche,sokobanX,sokobanY);
                zoomer(touche,&zoom);
                system("clear");
                affiche_entete(nomNiveau, compteur);
                afficher_plateau(plateau, niveau, zoom);
            }

            if (touche == RECOMMENCER){ // verification pour recommencer
                printf("Etes vous sur de vouloir recommencer [Y/N] ? ");
                scanf("%c", &verifRecommencer);

                if (verifRecommencer == 'Y'){
                    break;
                }
            }
            if (touche == ABANDON){ // verification abandon + enregistrer
                recommencer = false;
                printf("Voulez vous enregistrer votre partie [Y/N] ? ");
                scanf("%c", &enregistrer);

                if (enregistrer == 'Y'){
                    char nomFichier[30];
                    printf("nom du fichier : ");
                    scanf(" %s", nomFichier);
                    enregistrer_partie(plateau, nomFichier);
                }
                printf("Voulez vous enregistrer vos deplacements [Y/N] ? ");
                scanf(" %c", &enregistrerDep);

                if (enregistrerDep == 'Y'){
                    char nomFichierDep[30];
                    printf("nom du fichier (a la fin ajoutez \".dep\"): ");
                    scanf("%s", nomFichierDep);
                    enregistrer_deplacements(deplacements, compteur, nomFichierDep);
                }
                printf("Au revoir...\n");
            }
        }
    }
    if (victoire == true){ // victoire + enregistrer
        printf("Bravo !!! passez au niveau suivant !\n");
        printf("Voulez vous enregistrer vos deplacements [Y/N] ? ");
        scanf(" %c", &enregistrerDep);
        if (enregistrerDep == 'Y'){
            char nomFichierDep[30];
            printf("nom du fichier (a la fin ajoutez \".dep\"): ");
            scanf("%s", nomFichierDep);
            enregistrer_deplacements(deplacements, compteur, nomFichierDep);
        }
    }
    return EXIT_SUCCESS;
}

void lecture_niveau(char niveau[]){
    printf("nom du fichier .sok : ");
    scanf("%s", niveau);
}

int kbhit(){
    // la fonction retourne :
    // 1 si un caractere est present
    // 0 si pas de caractere présent
    int unCaractere = 0;
    struct termios oldt, newt;
    int ch;
    int oldf;

    // mettre le terminal en mode non bloquant
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);

    ch = getchar();

    // restaurer le mode du terminal
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    fcntl(STDIN_FILENO, F_SETFL, oldf);

    if (ch != EOF){
        ungetc(ch, stdin);
        unCaractere = 1;
    }
    return unCaractere;
}

void charger_partie(t_Plateau plateau, char fichier[]){
    FILE *f;
    char finDeLigne;

    f = fopen(fichier, "r");
    if (f == NULL){
        printf("ERREUR SUR FICHIER");
        exit(EXIT_FAILURE);
    }
    else{
        for (int ligne = 0; ligne < TAILLE; ligne++){
            for (int colonne = 0; colonne < TAILLE; colonne++){
                fread(&plateau[ligne][colonne], sizeof(char), 1, f);
            }
            fread(&finDeLigne, sizeof(char), 1, f);
        }
        fclose(f);
    }
}

void enregistrer_partie(t_Plateau plateau, char fichier[]){
    FILE *f;
    char finDeLigne = '\n';

    f = fopen(fichier, "w");
    for (int ligne = 0; ligne < TAILLE; ligne++){
        for (int colonne = 0; colonne < TAILLE; colonne++){
            fwrite(&plateau[ligne][colonne], sizeof(char), 1, f);
        }
        fwrite(&finDeLigne, sizeof(char), 1, f);
    }
    fclose(f);
}

void afficher_plateau(t_Plateau plateau, t_Plateau niveau, int zoom){
    char caseAffiche;
    caseAffiche = VIDE[0];

    for (int x = 0; x < TAILLE; x++){
        for (int i = 0; i < zoom; i++){ // affiche plusieurs fois les lignes en fonction du zoom
            for (int y = 0; y < TAILLE; y++){
                char casePlateau[1], caseNiveau[1];
                casePlateau[0] = plateau[x][y];
                caseNiveau[0] = niveau[x][y];
                if (casePlateau[0] == MURS[0]){
                    caseAffiche = MURS[0];
                }
                else if (casePlateau[0] == CAISSES[0] || casePlateau[0] == CAISSES_SUR_CIBLES[0]){
                    caseAffiche = CAISSES[0];
                    plateau[x][y] = CAISSES[0];
                }
                else if (casePlateau[0] == SOKOBAN[0] || casePlateau[0] == SOKOBAN_SUR_CIBLE[0])
                {
                    caseAffiche = SOKOBAN[0];
                }
                else if (caseNiveau[0] == CIBLES[0] || caseNiveau[0] == CAISSES_SUR_CIBLES[0] || caseNiveau[0] == SOKOBAN_SUR_CIBLE[0])
                {
                    if (casePlateau[0] != SOKOBAN[0] && casePlateau[0] != CAISSES[0]){
                        caseAffiche = CIBLES[0];
                        plateau[x][y] = CIBLES[0];
                    }
                }
                else{
                    caseAffiche = VIDE[0];
                }
                for (int k = 0; k < zoom; k++){ // affiche les colones plusieurs fois en fonction du zoom
                    printf("%c", caseAffiche);
                }
            }
            printf("\n");
        }
    }
}

void affiche_entete(char niveau[], int compteur){
    printf("SOKOBAN niveau : %s\n\ntouches de depalcements :\n%c (haut)", niveau, HAUT);
    printf(" %c (gauche)\n%c (bas)  %c (droite)\n\n", GAUCHE, BAS, DROITE);
    printf("Autre : %c (abandon) | %c (recommencer) | %c (zoom plus) | %c (zoom moins) | %c (undo) |\n\n", ABANDON, RECOMMENCER, PLUS, MOINS, UNDO);
    printf("Nombre de deplacements : %d \n\n\n", compteur);
}

void lecture_touches(char *Adr_touche){
    *Adr_touche = getchar();
}

void deplacer(char touche, t_Plateau plateau, int x, int y, int *adrCompteur, t_tabDeplacement deplacements){
    if (touche == HAUT && x > 0 && plateau[x - 1][y] != MURS[0]){
        if (!(plateau[x - 1][y] == CAISSES[0] &&
              (plateau[x - 2][y] == MURS[0] || plateau[x - 2][y] == CAISSES[0]))){
            if (plateau[x - 1][y] == CAISSES[0]){
                plateau[x - 1][y] = SOKOBAN[0];
                plateau[x - 2][y] = CAISSES[0];
                deplacements[*adrCompteur + 1] = CAISSE_HAUT;
            }
            else{
                plateau[x - 1][y] = SOKOBAN[0];
                deplacements[*adrCompteur + 1] = SOK_HAUT;
            }
            plateau[x][y] = VIDE[0];
            *adrCompteur = *adrCompteur + 1;
        }
    }
    else if (touche == GAUCHE && y > 0 && plateau[x][y - 1] != MURS[0]){
        if (!(plateau[x][y - 1] == CAISSES[0] &&
              (plateau[x][y - 2] == MURS[0] || plateau[x][y - 2] == CAISSES[0]))){
            if (plateau[x][y - 1] == CAISSES[0]){
                plateau[x][y - 1] = SOKOBAN[0];
                plateau[x][y - 2] = CAISSES[0];
                deplacements[*adrCompteur + 1] = CAISSE_GAUCHE;
            }
            else{
                plateau[x][y - 1] = SOKOBAN[0];
                deplacements[*adrCompteur + 1] = SOK_GAUCHE;
            }
            plateau[x][y] = VIDE[0];
            *adrCompteur = *adrCompteur + 1;
        }
    }
    else if (touche == BAS && x < (TAILLE - 1) && plateau[x + 1][y] != MURS[0]){
        if (!(plateau[x + 1][y] == CAISSES[0] &&
              (plateau[x + 2][y] == MURS[0] || plateau[x + 2][y] == CAISSES[0]))){
            if (plateau[x + 1][y] == CAISSES[0]){
                plateau[x + 1][y] = SOKOBAN[0];
                plateau[x + 2][y] = CAISSES[0];
                deplacements[*adrCompteur + 1] = CAISSE_BAS;
            }
            else{
                plateau[x + 1][y] = SOKOBAN[0];
                deplacements[*adrCompteur + 1] = SOK_BAS;
            }
            plateau[x][y] = VIDE[0];
            *adrCompteur = *adrCompteur + 1;
        }
    }
    else if (touche == DROITE && y < (TAILLE - 1) && plateau[x][y + 1] != MURS[0]){
        if (!(plateau[x][y + 1] == CAISSES[0] &&
              (plateau[x][y + 2] == MURS[0] || plateau[x][y + 2] == CAISSES[0]))){
            if (plateau[x][y + 1] == CAISSES[0]){
                plateau[x][y + 1] = SOKOBAN[0];
                plateau[x][y + 2] = CAISSES[0];
                deplacements[*adrCompteur + 1] = CAISSE_DROITE;
            }
            else{
                plateau[x][y + 1] = SOKOBAN[0];
                deplacements[*adrCompteur + 1] = SOK_DROITE;
            }
            plateau[x][y] = VIDE[0];
            *adrCompteur = *adrCompteur + 1;
        }
    }
}

void detection_sokoban(t_Plateau plateau, int *AdrX, int *AdrY){
    int x, y;
    bool trouve = false;
    for (x = 0; x < TAILLE; x++){
        for (y = 0; y < TAILLE; y++){
            if (plateau[x][y] == SOKOBAN[0] || plateau[x][y] == SOKOBAN_SUR_CIBLE[0]){
                trouve = true;
                break;
            }
        }
        if (trouve){
            break;
        }
    }
    if (trouve){
        *AdrX = x;
        *AdrY = y;
    }
    else{
        printf("sokoban introuvable\n");
    }
}

bool gagne(t_Plateau plateau, t_Plateau niveau){
    bool victoire = true;
    for (int x = 0; x < TAILLE; x++){
        for (int y = 0; y < TAILLE; y++){
            if ((niveau[x][y] == CIBLES[0] || niveau[x][y] == SOKOBAN_SUR_CIBLE[0]) || niveau[x][y] == CAISSES_SUR_CIBLES[0]){
                if (plateau[x][y] != CAISSES[0]){
                    victoire = false;
                }
            }
        }
    }
    return victoire;
}

void zoomer(char touche, int *zoom){
    if (touche == PLUS && *zoom < 3){
        *zoom = *zoom + 1;
    }
    else if (touche == MOINS && *zoom > 1){
        *zoom = *zoom - 1;
    }
}

void undo(t_Plateau plateau,t_tabDeplacement deplacements, int *adrCompteur, char touche, int x, int y){
    if(touche == UNDO){
        if(deplacements[*adrCompteur] == SOK_BAS){ // si le dernier deplacement enregistrer est sokoban seul vers le bas alors
            plateau[x - 1][y] = SOKOBAN[0]; // remonter sokoban de 1
            plateau[x][y] = VIDE[0]; // rendre la case ou il était présent vide
            *adrCompteur = *adrCompteur - 1; // reduire le compteur de mouvements de 1
        }
        else if(deplacements[*adrCompteur] == SOK_HAUT){ 
            plateau[x + 1][y] = SOKOBAN[0];
            plateau[x][y] = VIDE[0];
            *adrCompteur = *adrCompteur - 1;
        }
        else if(deplacements[*adrCompteur] == SOK_DROITE){
            plateau[x][y - 1] = SOKOBAN[0];
            plateau[x][y] = VIDE[0];
            *adrCompteur = *adrCompteur - 1;
        }
        else if(deplacements[*adrCompteur] == SOK_GAUCHE){
            plateau[x][y + 1] = SOKOBAN[0];
            plateau[x][y] = VIDE[0];
            *adrCompteur = *adrCompteur - 1;
        }
        else if(deplacements[*adrCompteur] == CAISSE_BAS){ // si le dernier deplacement enregistrer est sokoban avec une caisse vers le bas alors
            plateau[x - 1][y] = SOKOBAN[0]; // remonter sokoban de 1
            plateau[x + 1][y] = VIDE[0]; // rendre la position de la caisse vide
            plateau[x][y] = CAISSES[0]; // mettre la caisse a l'emplacement de sokoban
            *adrCompteur = *adrCompteur - 1;
        }
        else if(deplacements[*adrCompteur] == CAISSE_HAUT){
            plateau[x + 1][y] = SOKOBAN[0];
            plateau[x - 1][y] = VIDE[0];
            plateau[x][y] = CAISSES[0];
            *adrCompteur = *adrCompteur - 1;
        }
        else if(deplacements[*adrCompteur] == CAISSE_DROITE){
            plateau[x][y - 1] = SOKOBAN[0];
            plateau[x][y + 1] = VIDE[0];
            plateau[x][y] = CAISSES[0];
            *adrCompteur = *adrCompteur - 1;
        }
        else if(deplacements[*adrCompteur] == CAISSE_GAUCHE){
            plateau[x][y + 1] = SOKOBAN[0];
            plateau[x][y - 1] = VIDE[0];
            plateau[x][y] = CAISSES[0];
            *adrCompteur = *adrCompteur - 1;
        }
    }
}

void enregistrer_deplacements(t_tabDeplacement t, int nb, char fic[]){
    FILE * f;

    f = fopen(fic, "w");
    fwrite(t,sizeof(char), nb, f);
    fclose(f);
}