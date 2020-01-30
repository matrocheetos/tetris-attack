#include <windows.h>
#include <gl/gl.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "SOIL.h"

//funcoes que convertem a linha e coluna da matriz em uma coordenada de [-1,1]
#define MAT2X(j) ((j)*0.15f-1)
#define MAT2Y(i) (0.9-(i)*0.15f)

//velocidade do cenario
#define MOVE 5

//tamanho da matriz
#define M 14
#define N 8

// ----------------------------------------------
// conjuntos de variaveis do jogo:

struct Jogador{
    int pontos;
    int x,y;
    int status; //se perdeu ou nao
    int combo;
    int movimentos;
};

struct DadosCenario{
    int matrizMapa[M][M]; //matrizMapa[M][N] utilizada p desenhar pecas
    int matrizDestroi[M][N][5]; //guarda uma matriz de pecas a ser destruida assim como o tipo de peca
    int matrizTempo[M][N]; //guarda o tempo restante para uma peca ser eliminada/terminar sua animacao
    int pecasJuntasLinha[5][M]; //vetor p verificar cada tipo de peca na linha
    int pecasJuntasColuna[5][N]; // ... na coluna
    int pecasStatus; //determina se o jogo esta parado em uma animacao ou se pode continuar
};

// ----------------------------------------------
// carregar texturas:

GLuint pecasTextura[10];
GLuint numerosTextura[10];
GLuint gradeTextura;
GLuint cenarioTextura[12][6];

static void desenhaSprite(int tipo, float coluna, float linha, GLuint tex);
static GLuint carregaArqTextura(char *str);

void carregaTexturas(){
    int i,j;
    char str[50];
    //pecas
    for(i=0; i<5; i++){
        sprintf(str,".//imagens//peca%i.png",i); // 0: vermelha, 1: amarela, 2: azul, 3: roxa, 4: verde
        pecasTextura[i] = carregaArqTextura(str);
    }
    //pecas eliminadas
    for(i=0; i<5; i++){
        sprintf(str,".//imagens//pecasEliminadas//peca%i.png",i); // 5: vermelha, 6: amarela, 7: azul, 8: roxa, 9: verde
        pecasTextura[i+5] = carregaArqTextura(str);
    }
    //numeros
    for(i=0; i<10; i++){
        sprintf(str,".//imagens//sprite%i.png",i);
        numerosTextura[i] = carregaArqTextura(str);
    }
    //grade
    sprintf(str,".//imagens//grade.png");
    gradeTextura = carregaArqTextura(str);

    //cenario
    for(i=0;i<12;i++){
        for(j=0;j<6;j++){
            sprintf(str,".//imagens//cenario//%i-%i.png",j,i);
            cenarioTextura[i][j] = carregaArqTextura(str);
        }
    }
}

// funcao que carrega um arquivo de textura do jogo
static GLuint carregaArqTextura(char *str){
    // http://www.lonesock.net/soil.html
    GLuint tex = SOIL_load_OGL_texture
        (
            str,
            SOIL_LOAD_AUTO,
            SOIL_CREATE_NEW_ID,
            SOIL_FLAG_MIPMAPS | SOIL_FLAG_INVERT_Y |
            SOIL_FLAG_NTSC_SAFE_RGB | SOIL_FLAG_COMPRESS_TO_DXT
        );

    /* check for an error during the load process */
    if(0 == tex){
        printf( "SOIL loading error: '%s'\n", SOIL_last_result() );
    }

    return tex;
}

// Função que recebe uma linha e coluna da matriz e um código
// de textura e desenha um quadrado na tela com essa textura
void desenhaSprite(int tipo, float coluna, float linha, GLuint tex){
    float sprite;
    switch(tipo)
    {
        case 0: sprite=0.15f; break; //pecas
        case 1: sprite=0.17f; break; //grade
        case 2: sprite=0.155f; break; //cenario
        case 3: sprite=0.15f; break; //numeros
    }

    glColor3f(1.0, 1.0, 1.0);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, tex);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glBegin(GL_QUADS);
        glTexCoord2f(0.0f,0.0f); glVertex2f(coluna, linha);
        glTexCoord2f(1.0f,0.0f); glVertex2f(coluna+sprite, linha);
        glTexCoord2f(1.0f,1.0f); glVertex2f(coluna+sprite, linha+sprite);
        glTexCoord2f(0.0f,1.0f); glVertex2f(coluna, linha+sprite);
    glEnd();

    glDisable(GL_BLEND);
    glDisable(GL_TEXTURE_2D);
}

// ----------------------------------------------
// cenario:

struct DadosCenario* cenarioInicia(){
    struct DadosCenario* cen = malloc(sizeof(struct DadosCenario));
    int i,j;
    for(i=0; i<M; i++)              //preenche toda a matriz com -2
        for(j=0; j<N; j++)
            cen->matrizMapa[i][j]=-2;
    for(i=1; i<M-1; i++)            //preenche toda a matriz com -1, menos as bordas
        for(j=1; j<N-1; j++)
            cen->matrizMapa[i][j]=-1;

    cen->pecasStatus=1;

    return cen;
}

//percorre a matriz do jogo desenhando o cenario
void cenarioDesenha(struct DadosCenario* cen){
    int i,j;
    for(i=1; i<=12; i++)
        for(j=1; j<=12; j++){
            if(cen->matrizMapa[i][j]==-1){
                desenhaSprite(2,MAT2X(j),MAT2Y(i), cenarioTextura[i-1][j-1]);
            }
        }
}

//define um intervalo de tempo entre o movimento do cenario
void cenarioMovimentaA(struct DadosCenario *cen, struct Jogador *grade, time_t *tempo){
    if(grade->status==0)
        return;

    int diferencaTempo;
    time_t tempoAtual=time(NULL);

    diferencaTempo = tempoAtual - *tempo;

    if(diferencaTempo>=MOVE){
        cenarioMovimentaB(cen,grade);
        *tempo = time(NULL); //atualiza o tempo desde o ultimo movimento
    }
}

//faz as pecas subirem
void cenarioMovimentaB(struct DadosCenario *cen, struct Jogador *grade){
    int i,j,pecaAleatoria,novoMapa[M][N];

    for(i=0;i<M;i++)
        for(j=0;j<N;j++)
            novoMapa[i][j]=cen->matrizMapa[i][j];

    for(i=1;i<M-1;i++)
        for(j=1;j<N-1;j++)
            cen->matrizMapa[i-1][j]=novoMapa[i][j];

    for(j=1;j<N-1;j++){
        pecaAleatoria=rand()%5;
        //nao permite gerar uma sequencia pronta na linha:
        if(cen->matrizMapa[M-2][j-1]==pecaAleatoria){
            if(cen->matrizMapa[i-1][j-2]==pecaAleatoria)
                j--;
            else
                cen->matrizMapa[M-2][j]=pecaAleatoria;
        }else
            cen->matrizMapa[M-2][j]=pecaAleatoria;
        }

    //faz a grade andar junto com o cenario:
    grade->x = grade->x-1;
    grade->movimentos++;
}

void destroiCenario(struct DadosCenario *cen){
    free(cen);
}

// ----------------------------------------------
// controle da grade

// funcao que inicializa os dados associados a grade
struct Jogador* gradeJogador(int x, int y){
    struct Jogador* grade = malloc(sizeof(struct Jogador));
    if(grade != NULL){
        grade->pontos = 0;
        grade->status = 1;
        grade->combo = 0;
        grade->movimentos = 0;
        grade->x = x;
        grade->y = y;
    }
    return grade;
}

void gradeDesenha(struct Jogador *grade){
    int linha = grade->x;
    int coluna = grade->y;

    desenhaSprite(1,((coluna)*0.148f-1),(0.9-(linha)*0.151f), gradeTextura);
    desenhaSprite(1,((coluna+1)*0.148f-1),(0.9-(linha)*0.151f), gradeTextura);
}

//recebe input do teclado e muda a posicao da grade
void gradeMovimenta(struct Jogador *grade, int direcao, struct DadosCenario *cen){
    if(grade->status == 0)
        return;

    switch (direcao)
    {
        case 0: //direita
            if(cen->matrizMapa[grade->x][grade->y+2] != -2)
                grade->y++;
            break;
        case 1: //esquerda
            if(cen->matrizMapa[grade->x][grade->y-1] != -2)
                grade->y--;
            break;
        case 2: //cima
            if(cen->matrizMapa[grade->x-1][grade->y] != -2)
                grade->x--;
            break;
        case 3: //baixo
            if(cen->matrizMapa[grade->x+1][grade->y] != -2)
                grade->x++;
            break;

        default:
            break;
    }
}

// troca as posicoes das pecas dentro da grade
void gradeTrocaPeca(struct Jogador *grade, struct DadosCenario *cen){
    grade->movimentos++;

    int peca1 = cen->matrizMapa[grade->x][grade->y];
    int peca2 = cen->matrizMapa[grade->x][grade->y+1];
    cen->matrizMapa[grade->x][grade->y] = peca2;
    cen->matrizMapa[grade->x][grade->y+1] = peca1;
}

void destroiGrade(struct Jogador *grade){
    free(grade);
}

// ----------------------------------------------
// pecas

//gera pecas aleatorias no comeco do jogo
void pecaRandomiza(struct DadosCenario* cen){
    int i,j,pecaAleatoria;
    for(i=M/2;i<M-1;i++)
        for(j=1;j<N-1;j++){
            pecaAleatoria=rand()%5; //escolhe uma peca aleatoria de 0 a 4
            if(pecaAleatoria==cen->matrizMapa[i-1][j] || pecaAleatoria==cen->matrizMapa[i][j-1]){
                if(pecaAleatoria==cen->matrizMapa[i-2][j] || pecaAleatoria==cen->matrizMapa[i][j-2])
                    j--; //nao permite que a peca seja escolhida ja formando uma sequencia
                else
                    cen->matrizMapa[i][j]=pecaAleatoria;
            }else
                cen->matrizMapa[i][j]=pecaAleatoria;
        }
}

//percorre a matriz do jogo desenhando as pecas
void pecaDesenha(struct DadosCenario* cen){
    int i,j;
    for(i=1; i<M-1; i++)
        for(j=1; j<N-1; j++)
            desenhaSprite(0,MAT2X(j),MAT2Y(i),pecasTextura[cen->matrizMapa[i][j]]);
}

//verifica e elimina pecas em sequencia
void verificaPeca(struct Jogador *grade, struct DadosCenario *cen){
    int i,j,k,tipoPeca,jogada;

    //matrizDestroi: armazena as posicoes a serem eliminadas, necessaria para quando o jogador destroi linha e coluna conectadas
    for(k=0;k<5;k++)
        for(i=0;i<M;i++)
            for(j=0;j<N;j++)
                cen->matrizDestroi[i][j][k]=-2;

    if(grade->status==0)
        return;

    //limpa verificacao anterior
    for(tipoPeca=0;tipoPeca<5;tipoPeca++){
        for(i=0;i<M;i++){
            for(j=0;j<N;j++){
                cen->pecasJuntasLinha[tipoPeca][i]=0;
                cen->pecasJuntasColuna[tipoPeca][j]=0;
            }
        }
    }
    //verifica se perdeu
    for(j=1;j<N-1;j++)
        if(cen->matrizMapa[0][j]>-1){
            gameOver(cen,grade);
            break;
        }

    for(tipoPeca=0;tipoPeca<5;tipoPeca++){
        // linha
        for(i=0;i<M;i++){
            for(j=0;j<N;j++){
                if(cen->matrizMapa[i][j]==tipoPeca && cen->matrizMapa[i+1][j]!=-1){ //conta quantas pecas iguais estao conectadas e exclui aquelas que estao "caindo"
                    cen->pecasJuntasLinha[tipoPeca][i]++;
					cen->pecasJuntasColuna[tipoPeca][j]++;
                }else{
					if(cen->pecasJuntasColuna[tipoPeca][j]>=3){
                        for(k=1;k<=cen->pecasJuntasColuna[tipoPeca][j] ;k++)
                            cen->matrizDestroi[i-k][j][tipoPeca]=-1;   //define a posicao a ser eliminada
                        //grade->pontos += (cen->pecasJuntasColuna[tipoPeca][j]-2)*50;    //adiciona pontuacao
						grade->combo=1;
						jogada=grade->movimentos;
						if(grade->movimentos==jogada)
                            grade->combo=11;
                    }
                    if(cen->pecasJuntasLinha[tipoPeca][i]>=3){
                        for(k=1;k<=cen->pecasJuntasLinha[tipoPeca][i];k++)
                            cen->matrizDestroi[i][j-k][tipoPeca]=-1;   //define a posicao a ser eliminada
                        //grade->pontos += (cen->pecasJuntasLinha[tipoPeca][i]-2)*50;    //adiciona pontuacao
						grade->combo=10;
						if(grade->movimentos==jogada)
                            grade->combo=11;
                    }
					switch(grade->combo){
						case 10: grade->pontos += (cen->pecasJuntasLinha[tipoPeca][i]-2)*50; break;
						case 1: grade->pontos += (cen->pecasJuntasColuna[tipoPeca][j]-2)*50; break;
						case 11: grade->pontos += (cen->pecasJuntasLinha[tipoPeca][i]+cen->pecasJuntasColuna[tipoPeca][j]-2)*50; break;
						default: break;
					}
                    cen->pecasJuntasLinha[tipoPeca][i]=0;
					cen->pecasJuntasColuna[tipoPeca][j]=0;
                }
                grade->combo=0;
            }
        }
    }
}

//muda a textura das pecas p uma destruida
void eliminaPeca(struct DadosCenario* cen, struct Jogador *grade){
    int i,j,tipoPeca;
    time_t tempoSegundos;

    for(tipoPeca=0;tipoPeca<5;tipoPeca++){
        for(i=0;i<M;i++){
            for(j=0;j<N;j++){
                if(cen->matrizDestroi[i][j][tipoPeca]==-1){
                    cen->matrizMapa[i][j] = tipoPeca+5; //pecas eliminadas ocupam valores de 5 a 9
                    tempoSegundos = time(NULL);
                    cen->matrizTempo[i][j] = tempoSegundos;
                }
            }
        }
    }
    destroiPeca(cen,grade);
}

//tira a peca do cenario
void destroiPeca(struct DadosCenario* cen, struct Jogador *grade){
    int i,j,diferencaTempo;
    time_t tempoAtual = time(NULL);

    for(i=0;i<M;i++){
        for(j=0;j<N;j++){
            if(cen->matrizMapa[i][j]>4){
                diferencaTempo = tempoAtual - cen->matrizTempo[i][j];
                if(diferencaTempo>=1){
                    cen->matrizMapa[i][j]=-1;
                    //verifica se o jogo terminou e o cenario esta vazio
                    if(grade->status==0){
                        int m,n;
                        for(m=1;m<M-1;m++)
                            for(n=1;n<N-1;n++)
                                if(cen->matrizMapa[m][n]!=-1)
                                    grade->status=0;
                                else
                                    grade->status=2;
                    }
                }
            }
        }
    }
    if(grade->status==2)
        PostQuitMessage(0); //termina o jogo
}

//puxa as pecas p baixo
void gravidadePeca(struct DadosCenario *cen, struct Jogador *grade){
    int i,j;
    for(i=1;i<M-1;i++)
        for(j=1;j<N-1;j++){
            if(cen->matrizMapa[i][j]>-1){
                if(cen->matrizMapa[i+1][j]==-1){ //procura vazio na linha de baixo
                    cen->matrizMapa[i+1][j]=cen->matrizMapa[i][j]; //passa a peca p baixo
                    cen->matrizMapa[i][j]=-1;
                    grade->movimentos++;
                }
            }
        }
}

// ---------------------------------
// pontuacao

void pontuacaoDesenha(struct DadosCenario *cen, struct Jogador *grade){
    int i,pontuacao;
    int casaPontos[5]; //unidade, dezena, centena, ...
    pontuacao = grade->pontos;

    //armazena a pontuacao no vetor:
    for(i=4;i>=0;i--){
        casaPontos[i] = pontuacao%10;
        pontuacao = pontuacao/10;
    }

    for(i=4;i>=0;i--)
        desenhaSprite(3,((N+i)*0.15f-1),(0.9-(2)*0.15f),numerosTextura[casaPontos[i]]);
}

// ---------------------------------
// fim do jogo

void gameOver(struct DadosCenario *cen, struct Jogador *grade){
    int i,j;
    grade->status=0;
    for(i=1;i<M-1;i++){
        for(j=1;j<N-1;j++){
            if(cen->matrizMapa[i][j]!=-1){
                cen->matrizDestroi[i][j][cen->matrizMapa[i][j]]=-1;
            }
        }
    }
}
