#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <termios.h>
#include <unistd.h>

#define JOGADOR_1 'X'
#define JOGADOR_2 'O'
#define TAMANHO_MENSAGEM 21

//
// typedefs
//
typedef struct {
  int y;
  int x;
} Coord;

typedef struct {
  Coord cima;
  Coord direita;
  Coord baixo;
  Coord esquerda;
} Movimentos;

typedef struct {
  char vez_do_jogador;
  Coord posicao_atual;
  char cerquilha[3][3];
  char mensagens[3][TAMANHO_MENSAGEM];
} ContextoJogo;

//
// Variáveis Globais
//
Movimentos MOVIMENTOS = {.cima = {.y = -1, .x = 0},
                         .direita = {.y = 0, .x = 1},
                         .baixo = {.y = 1, .x = 0},
                         .esquerda = {.y = 0, .x = -1}};

struct termios orig_termios;

//
// Funções
//
void POS_Mover(Coord *posicao, Coord *direcao) {
  int x_apos_movimento = posicao->x + direcao->x;
  int y_apos_movimento = posicao->y + direcao->y;
  if (((x_apos_movimento < 0) || (y_apos_movimento < 0)) ||
      ((x_apos_movimento > 2) || (y_apos_movimento > 2))) {
    return;
  }

  posicao->x = x_apos_movimento;
  posicao->y = y_apos_movimento;
  return;
}

bool CQL_Marcar(char cerquilha[3][3], Coord *posicao_escolhida, char marcador) {
  char casa = cerquilha[posicao_escolhida->y][posicao_escolhida->x];
  if (casa == ' ') {
    cerquilha[posicao_escolhida->y][posicao_escolhida->x] = marcador;
    return true;
  }

  return false;
}

void CQL_Print(char cerquilha[3][3], Coord *posicao_escolhida) {
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 3; j++) {
      printf("---");
    }
    printf("\n");
    for (int j = 0; j < 3; j++) {
      if ((i == posicao_escolhida->y) && (j == posicao_escolhida->x)) {
        printf(">%c<", cerquilha[i][j]);
      } else {
        printf("|%c|", cerquilha[i][j]);
      }
    }
    printf("\n");
  }

  for (int j = 0; j < 3; j++) {
    printf("---");
  }
  printf("\n");
}

bool CQL_VitoriaFoiObtida(char cerquilha[3][3]) {
  char x;
  char y;
  char z;
  for (int i = 0; i < 3; i++) {
    x = cerquilha[i][0];
    y = cerquilha[i][1];
    z = cerquilha[i][2];

    if ((x != ' ') && (x == y) && (y == z) && (z == x)) {
      return true;
    }

    x = cerquilha[0][i];
    y = cerquilha[1][i];
    z = cerquilha[2][i];

    if ((x != ' ') && (x == y) && (y == z) && (z == x)) {
      return true;
    }
  }

  x = cerquilha[0][0];
  y = cerquilha[1][1];
  z = cerquilha[2][2];
  if ((x != ' ') && (x == y) && (y == z) && (z == x)) {
    return true;
  }

  x = cerquilha[0][2];
  y = cerquilha[1][1];
  z = cerquilha[2][0];
  if ((x != ' ') && (x == y) && (y == z) && (z == x)) {
    return true;
  }

  return false;
}

void CTX_Init(ContextoJogo *ctx) {
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 3; j++) {
      ctx->cerquilha[i][j] = ' ';
    }
  }

  ctx->posicao_atual.y = 1;
  ctx->posicao_atual.x = 1;

  ctx->vez_do_jogador = JOGADOR_1;

  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < TAMANHO_MENSAGEM; j++) {
      ctx->mensagens[i][j] = ' ';
    }
    ctx->mensagens[i][TAMANHO_MENSAGEM - 1] = '\0';
  }
}

void MSG_Colocar(ContextoJogo *ctx, char *mensagem) {
  for (int i = 0; i < TAMANHO_MENSAGEM; i++) {
    ctx->mensagens[2][i] = ctx->mensagens[1][i];
    ctx->mensagens[1][i] = ctx->mensagens[0][i];
  }

  int k = 0;
  while (mensagem[k] != '\0') {
    ctx->mensagens[0][k] = mensagem[k];
    k++;
  }
  for (int i = k; i < TAMANHO_MENSAGEM; i++) {
    ctx->mensagens[0][i] = ' ';
  }
  ctx->mensagens[0][TAMANHO_MENSAGEM - 1] = '\0';
}

void MSG_UltimaJogada(ContextoJogo *ctx) {
  char msg_marcar[TAMANHO_MENSAGEM];
  char t[] = "%c - %d %d";
  snprintf(msg_marcar, TAMANHO_MENSAGEM, t, ctx->vez_do_jogador,
           ctx->posicao_atual.y, ctx->posicao_atual.x);

  MSG_Colocar(ctx, msg_marcar);
}

void MSG_Vitoria(ContextoJogo *ctx) {
  char msg_marcar[TAMANHO_MENSAGEM];
  char t[] = "%c Venceu!";
  snprintf(msg_marcar, TAMANHO_MENSAGEM, t, ctx->vez_do_jogador);

  MSG_Colocar(ctx, msg_marcar);
}

void MSG_Print(ContextoJogo *ctx) {
  for (int i = 0; i < TAMANHO_MENSAGEM; i++) {
    printf("-");
  }
  printf("\n");
  for (int i = 0; i < 3; i++) {
    printf("%s\n", ctx->mensagens[i]);
  }
  for (int i = 0; i < TAMANHO_MENSAGEM; i++) {
    printf("-");
  }
  printf("\n");
}

void PrintJogo(ContextoJogo *ctx) {
  write(STDIN_FILENO, "\x1b[2J", 4);
  printf("\n");
  printf("Vez do %c\n", ctx->vez_do_jogador);
  CQL_Print(ctx->cerquilha, &ctx->posicao_atual);
  MSG_Print(ctx);
}

void TerminalReset() { tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios); }

void TerminalSetup() {
  tcgetattr(STDIN_FILENO, &orig_termios);
  atexit(TerminalReset);

  struct termios term_alterado = orig_termios;
  term_alterado.c_lflag &= ~(ECHO | ICANON);

  tcsetattr(STDIN_FILENO, TCSAFLUSH, &term_alterado);
}

//
// Main
//
int main() {
  TerminalSetup();

  ContextoJogo ctx;
  CTX_Init(&ctx);
  write(STDIN_FILENO, "\x1b[2J", 4);
  CQL_Print(ctx.cerquilha, &ctx.posicao_atual);

  while (1) {
    int n;
    char c;
    while ((n = read(STDIN_FILENO, &c, 1)) != 1) {
      if (n == -1 && errno != EAGAIN) {
        perror("read");
      }
    }
    if (c == 'q') {
      return 0;
    } else if (c == '\n') {
      if (CQL_Marcar(ctx.cerquilha, &ctx.posicao_atual, ctx.vez_do_jogador)) {
        MSG_UltimaJogada(&ctx);
        if (CQL_VitoriaFoiObtida(ctx.cerquilha)) {
          MSG_Vitoria(&ctx);
          break;
        }

        if (ctx.vez_do_jogador == JOGADOR_1) {
          ctx.vez_do_jogador = JOGADOR_2;
        } else {
          ctx.vez_do_jogador = JOGADOR_1;
        }
      }
    } else if (c == '\x1b') {
      char seq[3];

      read(STDIN_FILENO, &seq[0], 1);
      read(STDIN_FILENO, &seq[1], 1);

      if (seq[0] == '[') {
        switch (seq[1]) {
        case 'A':
          POS_Mover(&ctx.posicao_atual, &MOVIMENTOS.cima);
          break;
        case 'B':
          POS_Mover(&ctx.posicao_atual, &MOVIMENTOS.baixo);
          break;
        case 'C':
          POS_Mover(&ctx.posicao_atual, &MOVIMENTOS.direita);
          break;
        case 'D':
          POS_Mover(&ctx.posicao_atual, &MOVIMENTOS.esquerda);
          break;
        }
      }
    }

    //
    // Print
    //
    PrintJogo(&ctx);
  }
  PrintJogo(&ctx);

  return 0;
}
