#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

#define JOGADOR_1 'X'
#define JOGADOR_2 'O'

struct termios orig_termios;

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
  char mensagens[3][21];
} ContextoJogo;

Movimentos MOVIMENTOS = {.cima = {.y = -1, .x = 0},
                         .direita = {.y = 0, .x = 1},
                         .baixo = {.y = 1, .x = 0},
                         .esquerda = {.y = 0, .x = -1}};

struct termios orig_termios;

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
  write(STDIN_FILENO, "\x1b[2J", 4);
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
}

void TerminalReset() { tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios); }

void TerminalSetup() {
  tcgetattr(STDIN_FILENO, &orig_termios);
  atexit(TerminalReset);

  struct termios term_alterado = orig_termios;
  term_alterado.c_lflag &= ~(ECHO | ICANON);

  tcsetattr(STDIN_FILENO, TCSAFLUSH, &term_alterado);
}

int main() {
  TerminalSetup();

  ContextoJogo ctx;
  CTX_Init(&ctx);
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
        CQL_Print(ctx.cerquilha, &ctx.posicao_atual);
        if (CQL_VitoriaFoiObtida(ctx.cerquilha)) {
          printf("%c Venceu!\n", ctx.vez_do_jogador);
          return 0;
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
          CQL_Print(ctx.cerquilha, &ctx.posicao_atual);
          break;
        case 'B':
          POS_Mover(&ctx.posicao_atual, &MOVIMENTOS.baixo);
          CQL_Print(ctx.cerquilha, &ctx.posicao_atual);
          break;
        case 'C':
          POS_Mover(&ctx.posicao_atual, &MOVIMENTOS.direita);
          CQL_Print(ctx.cerquilha, &ctx.posicao_atual);
          break;
        case 'D':
          POS_Mover(&ctx.posicao_atual, &MOVIMENTOS.esquerda);
          CQL_Print(ctx.cerquilha, &ctx.posicao_atual);
          break;
        }
      }
    }
  }

  return 0;
}
