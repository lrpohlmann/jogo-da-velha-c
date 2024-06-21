#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <termios.h>
#include <unistd.h>

#define JOGADOR_1 'X'
#define JOGADOR_2 'O'

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
} ContextoJogo;

//
// Variáveis Globais
//
Movimentos MOVIMENTOS = {.cima = {.y = -1, .x = 0},
                         .direita = {.y = 0, .x = 1},
                         .baixo = {.y = 1, .x = 0},
                         .esquerda = {.y = 0, .x = -1}};

struct termios orig_termios;

enum CondicaoJogo {
  EM_ANDAMENTO = 0,
  VITORIA,
  EMPATE,
};
//
// Funções
//

void crash(const char *e) {
  write(STDOUT_FILENO, "\x1b[2J", 4);
  write(STDOUT_FILENO, "\x1b[H", 3);

  perror(e);
  exit(1);
}

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

enum CondicaoJogo VitoriaOuEmpate(ContextoJogo *ctx) {
  {
    char x;
    char y;
    char z;
    for (int i = 0; i < 3; i++) {
      x = ctx->cerquilha[i][0];
      y = ctx->cerquilha[i][1];
      z = ctx->cerquilha[i][2];

      if ((x != ' ') && (x == y) && (y == z) && (z == x)) {
        return VITORIA;
      }

      x = ctx->cerquilha[0][i];
      y = ctx->cerquilha[1][i];
      z = ctx->cerquilha[2][i];

      if ((x != ' ') && (x == y) && (y == z) && (z == x)) {
        return VITORIA;
      }
    }

    x = ctx->cerquilha[0][0];
    y = ctx->cerquilha[1][1];
    z = ctx->cerquilha[2][2];
    if ((x != ' ') && (x == y) && (y == z) && (z == x)) {
      return VITORIA;
    }

    x = ctx->cerquilha[0][2];
    y = ctx->cerquilha[1][1];
    z = ctx->cerquilha[2][0];
    if ((x != ' ') && (x == y) && (y == z) && (z == x)) {
      return VITORIA;
    }
  }

  {
    for (int i = 0; i < 3; i++) {
      for (int j = 0; j < 3; j++) {
        if (ctx->cerquilha[i][j] == ' ') {
          return EM_ANDAMENTO;
        }
      }
    }
    return EMPATE;
  }

  return EM_ANDAMENTO;
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

void PrintJogo(ContextoJogo *ctx) {
  write(STDIN_FILENO, "\x1b[2J", 4);
  write(STDIN_FILENO, "\x1b[1;1H", 6);
  int buffer_size = 126;
  char cql[buffer_size];
  int i =
      sprintf(cql,
              "Vez do "
              "%c\n---------\n|%c||%c||%c|\n---------\n|%c||%c||%c|\n---------"
              "\n|%c||%c||%c"
              "|\n---------\n"
              "<Setas> - Mover | <Enter> - Marcar | q - Sair\n",
              ctx->vez_do_jogador, ctx->cerquilha[0][0], ctx->cerquilha[0][1],
              ctx->cerquilha[0][2], ctx->cerquilha[1][0], ctx->cerquilha[1][1],
              ctx->cerquilha[1][2], ctx->cerquilha[2][0], ctx->cerquilha[2][1],
              ctx->cerquilha[2][2]);
  if (i < 0) {
    crash("sprintf");
  }

  int bytes_escritos = write(STDOUT_FILENO, &cql, buffer_size);
  if (bytes_escritos != buffer_size) {
    crash("write");
  }

  {
    int cursor_linha = 3 + 2 * ctx->posicao_atual.y;
    int cursor_coluna = 2 + 3 * ctx->posicao_atual.x;
    char comando_posicao_cursor[7];
    sprintf(comando_posicao_cursor, "\x1b[%d;%dH", cursor_linha, cursor_coluna);
    write(STDIN_FILENO, comando_posicao_cursor, 6);
  }
}

void TerminalReset() {
  write(STDIN_FILENO, "\x1b[2J", 4);
  write(STDIN_FILENO, "\x1b[1;1H", 6);
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

void TerminalSetup() {
  tcgetattr(STDIN_FILENO, &orig_termios);
  atexit(TerminalReset);

  struct termios term_alterado = orig_termios;
  term_alterado.c_lflag &= ~(ECHO | ICANON);

  tcsetattr(STDIN_FILENO, TCSAFLUSH, &term_alterado);
}

void Jogo() {
  ContextoJogo ctx;
  CTX_Init(&ctx);
  PrintJogo(&ctx);

  while (1) {
    int n;
    char c;
    while ((n = read(STDIN_FILENO, &c, 1)) != 1) {
      if (n == -1 && errno != EAGAIN) {
        crash("read");
      }
    }
    if (c == 'q') {
      exit(0);
    } else if (c == '\n') {
      if (CQL_Marcar(ctx.cerquilha, &ctx.posicao_atual, ctx.vez_do_jogador)) {
        if (VitoriaOuEmpate(&ctx) != EM_ANDAMENTO) {
          PrintJogo(&ctx);
          return;
        }

        if (ctx.vez_do_jogador == JOGADOR_1) {
          ctx.vez_do_jogador = JOGADOR_2;
        } else {
          ctx.vez_do_jogador = JOGADOR_1;
        }
      }
    } else if (c == '\x1b') {
      char seq[3];

      if (read(STDIN_FILENO, &seq[0], 1) == -1) {
        crash("read");
      };
      if (read(STDIN_FILENO, &seq[1], 1) == -1) {
        crash("read");
      }

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

    PrintJogo(&ctx);
  }
}

bool NovoJogo() {
  printf("\n");
  printf("Novo Jogo? <Enter> - Sim, <q> - Não\n");

  while (1) {
    int n;
    char c;
    while ((n = read(STDIN_FILENO, &c, 1)) != 1) {
      if (n == -1 && errno != EAGAIN) {
        crash("read");
      }
    }

    if (c == 'q') {
      return false;
    } else if (c == '\n') {
      return true;
    }
  }
}

//
// Main
//
int main() {
  TerminalSetup();

  bool jogar = true;
  while (jogar) {
    Jogo();
    jogar = NovoJogo();
  }
  return 0;
}
