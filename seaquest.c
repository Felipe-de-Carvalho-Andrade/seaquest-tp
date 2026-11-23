#include <stdio.h>
#include <stdlib.h>
#include <allegro5/allegro.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_ttf.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_image.h>
#include <math.h>
#include <time.h>


typedef struct Sub {
	float x, y;
	float vel;	
	int dir_x;
	int dir_y;
	float oxygen;
	int invulnerability;
	ALLEGRO_COLOR color;
} Sub;

typedef struct Diver {
	float x,y;
	int dir_x;
	float vel;
	int foe; // Indicador reservado para diferenciar mergulhadores e inimigos.
	ALLEGRO_COLOR color;
} Diver;


/* Frequencia de atualizacao do jogo, em quadros por segundo.
 * As velocidades das estruturas sao medidas em pixels por quadro. */
const float FPS = 60;

/* Dimensoes da janela, tamanho do submarino e velocidades previstas. */
const int SCREEN_W = 960;
const int SCREEN_H = 540;
const int SUB_HEIGHT = 20;
const int SUB_WIDTH = 80;
const float SUB_VEL_ABOVE = 5;
const float SUB_VEL_SHALLOW = 2.7;
const float SUB_VEL_SEA = 0.5;

/* Constantes reservadas para a implementacao dos objetos e do oxigenio.*/
#define NUM_DIVERS 12
const int DIVER_WIDTH = 8;
const int DIVER_HEIGHT = 3;
const int MAX_OXYGEN = 12;
const float OXYGEN_CONSUMPTION = 1.0;
const float OXYGEN_RECHARGE = 2.0;

/* Dimensoes previstas para inimigos; nao utilizadas nesta versao. */
const int FOE_WIDTH = 32;
const int FOE_HEIGTH = 14;

/* Altura da faixa superior da tela e pontuacao corrente. */
int SEA_LEVEL;
int SCORE = 0;
int LIVES = 3;

/* Recursos globais: cores e fonte para a interface.
 * SKY_COLOR e as distancias maximas de colisao estao reservadas
 * para extensoes do projeto e ainda nao sao utilizadas. */
ALLEGRO_COLOR SEA_COLOR;
ALLEGRO_COLOR SKY_COLOR;
ALLEGRO_FONT *size_32;
float MAX_DIST_COLLISION_DIVER;
float MAX_DIST_COLLISION_FOE;

/**
 * @brief Inicializa o nivel da superficie, as cores e a fonte global.
 *
 * Define SEA_LEVEL como um sexto da altura da tela, define as cores
 * SEA_COLOR e SKY_COLOR e carrega a fonte "arial.ttf" no tamanho 32.
 * @note A funcao nao verifica se o carregamento da fonte teve sucesso.
 */
void initGlobalVars() {
	SEA_LEVEL = SCREEN_H/6;
	SEA_COLOR = al_map_rgb(0,0,155);
	SKY_COLOR = al_map_rgb(10,10,10);
	size_32 = al_load_font("arial.ttf", 32, 1);   
}


/**
 * @brief Desenha o fundo azul, a faixa superior cinza e a pontuacao.
 * @param sub Estado do submarino; parametro ainda nao utilizado aqui.
 *
 * @note O vetor text_score comporta somente tres caracteres numericos
 *       e o terminador '\0'; o codigo original usa sprintf sem limite.
 */
void drawScenario(Sub sub) {
    al_clear_to_color(SEA_COLOR);   
    al_draw_filled_rectangle(0, 0, SCREEN_W, SEA_LEVEL, al_map_rgb(70, 70, 70)); 

    char text_score[14];
    sprintf(text_score, "%d", SCORE);
    al_draw_text(size_32, al_map_rgb(0, 200, 0), 10, 10, 0, text_score);

    // Dimensoes da barra de oxigenio
    float bar_x = 700.0;
    float bar_y = 18.0;
    float bar_w = 220.0;
    float bar_h = 22.0;

    float ratio = sub.oxygen / MAX_OXYGEN;

    al_draw_filled_rectangle(bar_x, bar_y, bar_x + bar_w, bar_y + bar_h, al_map_rgb(30, 30, 30));
    al_draw_filled_rectangle(bar_x, bar_y, bar_x + bar_w * ratio, bar_y + bar_h, al_map_rgb(0, 180, 255));
    al_draw_rectangle(bar_x, bar_y, bar_x + bar_w, bar_y + bar_h, al_map_rgb(255, 255, 255), 1.25);
}

/**
 * @brief Inicializa posicao, velocidade, direcoes, oxigenio e cor.
 * @param sub Ponteiro para o submarino a ser inicializado.
 *
 * O submarino comeca no limite da faixa superior, com velocidade
 * SUB_VEL_ABOVE, sem movimento e com o reservatorio cheio.
 */
void initSub(Sub *sub) {
	sub->x = 0;
	sub->y = SEA_LEVEL - SUB_HEIGHT/2;
	sub->vel = SUB_VEL_ABOVE;
	sub->dir_x = 0;
	sub->dir_y = 0;
	sub->oxygen = MAX_OXYGEN;
	sub->invulnerability = 0;
	sub->color = al_map_rgb(230, 230, 0);
}

/**
 * @brief Gera um numero inteiro aleatorio entre um valor maximo e um valor minimo
 */
int randInt(int min, int max) {
    return min + rand() % (max - min + 1);
}

/**
 * @brief Inicializa um unico objeto circular verde.
 * @param diver Ponteiro para o objeto a ser inicializado.
 */
void initDiver(Diver *diver) {
	diver->dir_x = randInt(0, 1) ? 1 : -1;

	if(diver->dir_x == 1) {
		diver->x = -30;
	} else {
		diver->x = SCREEN_W + 30;
	}
	
	// Surgimento dos objetos no mar com margem de seguranca superior e inferior
	int min_y = SEA_LEVEL + 20;
	int max_y = SCREEN_H - 15;

	diver->y = randInt(min_y, max_y);
	diver->vel = randInt(1,5);

	diver->foe = randInt(0,1);
	
	diver->color = diver->foe ?  al_map_rgb(255,0,0) : al_map_rgb(255,255,255);
}

/**
 * @brief Desenha o submarino como um retangulo preenchido arredondado.
 * @param sub Estado utilizado para obter a posicao e a cor do desenho.
 *
 * As dimensoes sao definidas pelas constantes SUB_WIDTH e SUB_HEIGHT.
 */
void drawSub(Sub sub) {

	al_draw_filled_rounded_rectangle(sub.x, sub.y, sub.x+SUB_WIDTH, sub.y+SUB_HEIGHT, SUB_WIDTH/3, SUB_HEIGHT/3, sub.color);

}

/**
 * @brief Retorna 1 se a posição y estiver na superfície ou acima, e 0 caso contrário.
 */
int isAboveSea(float pos_y) {
    return pos_y <= SEA_LEVEL;
}

/**
 * @brief Atualiza a velocidade do submarino dependendo da sua profundidade.
 */
void updateSpeed(Sub *s) {
	// Calcula centro do submarino em y
    float center_y = s->y + SUB_HEIGHT / 2.0f;

    if (isAboveSea(center_y)) {
        s->vel = SUB_VEL_ABOVE;
        return;
    }

    float p = (center_y - SEA_LEVEL) / ((SCREEN_H - SUB_HEIGHT / 2.0f) - SEA_LEVEL);

    s->vel = SUB_VEL_SHALLOW - p * (SUB_VEL_SHALLOW - SUB_VEL_SEA);
}
/**
 * @brief Restringe a posição do submarino aos limites da tela.
 */
void screenLimit(Sub *s) {
    if (s->x < 0) {
        s->x = 0;
    } else if (s->x > SCREEN_W - SUB_WIDTH) {
        s->x = SCREEN_W - SUB_WIDTH;
    }

	if (s->y < SEA_LEVEL - SUB_HEIGHT) {
        s->y = SEA_LEVEL - SUB_HEIGHT; // Impede o submarino de voar
    } else if (s->y > SCREEN_H - SUB_HEIGHT) {
        s->y = SCREEN_H - SUB_HEIGHT;
    }
}

/**
 * @brief Atualiza a posicao do submarino de acordo com o teclado.
 * @param s Ponteiro para o submarino a ser atualizado.
 */
void updateSub(Sub *s) {
	updateSpeed(s);

	s->x += s->dir_x * s->vel;
	s->y += s->dir_y * s->vel;

	screenLimit(s);
}


/**
 * @brief Desenha o objeto como um circulo preenchido.
 * @param diver Objeto que fornece centro e cor do circulo.
 *
 * O raio do circulo e fixo em 10 pixels; DIVER_WIDTH e
 * DIVER_HEIGHT ainda nao sao utilizados no desenho.
 */
void drawDiver(Diver diver) {
	if (!diver.foe) {
		al_draw_filled_circle(diver.x, diver.y, 10, diver.color);
	} else {
		al_draw_filled_rectangle(diver.x, 
			diver.y - FOE_HEIGTH / 2, 
            diver.x + FOE_WIDTH, 
            diver.y + FOE_HEIGTH / 2, diver.color
		);
	}

}

/**
 * @brief Verifica se o objeto ultrapassou completamente os limites laterais da tela.
 */
int isObjectOutOfBounds(Diver *d) {
    if (d->dir_x > 0 && d->x > SCREEN_W + 50) {
        return 1;
    } else if (d->dir_x < 0 && d->x < -50) {
        return 1; 
    }
    return 0;
}

/**
 * @brief Desloca um unico objeto horizontalmente para a direita.
 */
void updateDiver(Diver *diver) {
	diver->x += diver->dir_x * diver->vel;

	if(isObjectOutOfBounds(diver)) {
		initDiver(diver);
	}
}

/**
 * @brief Verifica se ocorreu colisao entre um submarino e um objeto
 */
int colisionDetected(Sub s, Diver d) {
    // Limites do Submarino
    float sub_left = s.x;
    float sub_right = s.x + SUB_WIDTH;
    float sub_top = s.y;
    float sub_bottom = s.y + SUB_HEIGHT;

    // Diferencia o objeto
    if (d.foe) {
        // Inimigo (Retângulo)
        float obj_left = d.x;
        float obj_right = d.x + FOE_WIDTH;
        float obj_top = d.y - FOE_HEIGTH / 2;
        float obj_bottom = d.y + FOE_HEIGTH / 2;

        // Verificar se NÃO há colisão
        if (sub_right < obj_left || sub_left > obj_right || sub_bottom < obj_top || sub_top > obj_bottom) {   
            return 0; 
        }

        return 1;
    } else {
        // Mergulhador (Círculo)
        float radius = 10.0;
        float closest_x = d.x;

        if (closest_x < sub_left) 
			closest_x = sub_left;
        else if (closest_x > sub_right) 
			closest_x = sub_right;

        float closest_y = d.y;
		
        if (closest_y < sub_top) 
			closest_y = sub_top;
        else if (closest_y > sub_bottom) 
			closest_y = sub_bottom;

        float dx = d.x - closest_x;
        float dy = d.y - closest_y;

        return (dx * dx + dy * dy) <= (radius * radius);
    }
}

/**
 * @brief Atualiza o oxigenio do submarino conforme sua posicao em relacao a superficie.
 */
void updateOxygen(Sub *s) {
    if (isAboveSea(s->y + (SUB_HEIGHT / 2))) {
        // Reabastece oxigenio gradualmente enquanto estiver na superficie
        s->oxygen += (OXYGEN_RECHARGE / FPS);

		if(s->oxygen > MAX_OXYGEN)
			s->oxygen = MAX_OXYGEN;
 
    } else {
        // Consome oxigenio gradualmente enquanto submerso
        s->oxygen -= OXYGEN_CONSUMPTION / FPS;
        if (s->oxygen < 0.0) {
            s->oxygen = 0.0;
        }
    }
}

void drawGame(Sub sub, Diver divers[]) {
	drawScenario(sub);
	drawSub(sub);
			
	for(int i = 0; i < NUM_DIVERS; i++) {
		drawDiver(divers[i]);
	}
} 

void updateGame(Sub *sub, Diver divers[], int *score, int *playing, int *lives) {
	updateSub(sub);
	updateOxygen(sub);

	if(sub->oxygen <= 0.0) {
		*playing = 0;
	}

	if(sub->invulnerability > 0) {
		sub->invulnerability--;
	}
	
	for(int i = 0; i < NUM_DIVERS; i++) {
		// Atualiza o objeto
		updateDiver(&divers[i]);
		
		// Verifica se houve colisão com o submarino
		if(colisionDetected(*sub, divers[i])) {
			if(divers[i].foe == 1 && sub->invulnerability <= 0) {
				(*lives)--;
				sub->invulnerability = 120;
				initDiver(&divers[i]);
			} 
			else if (divers[i].foe == 0) {
				(*score)++;
				initDiver(&divers[i]); // Reposiciona o mergulhador resgatado
			}
		}

		if((*lives) == 0) {
			*playing = 0;
			break;
		}
	}
}
 
/**
 * @brief Inicializa a Allegro e executa o laco principal do jogo.
 *
 * Inicializa os modulos graficos, o temporizador, a janela, o teclado
 * e a fila de eventos. Cria um Sub e um Diver. A cada evento do timer,
 * desenha o cenario, atualiza as duas posicoes e apresenta o quadro.
 * As teclas W, A, S e D controlam a direcao; fechar a janela encerra
 * o laco. Ao terminar, apresenta uma tela com a pontuacao corrente.
 *
 *
 * @param argc Quantidade de argumentos de linha de comando (nao usada).
 * @param argv Vetor de argumentos de linha de comando (nao usado).
 * @return 0 em caso de encerramento normal, ou -1 em determinadas
 *         falhas de inicializacao.
 */
int main(int argc, char **argv){
	srand(time(NULL));

	int i;

	ALLEGRO_DISPLAY *display = NULL;
	ALLEGRO_EVENT_QUEUE *event_queue = NULL;
	ALLEGRO_TIMER *timer = NULL;
   
	// ----------------------- Rotinas de inicializacao -----------------------
    
	// Inicializa a biblioteca Allegro.
	if(!al_init()) {
		fprintf(stderr, "failed to initialize allegro!\n");
		return -1;
	}
	
    // Inicializa o modulo de primitivas graficas da Allegro.
    if(!al_init_primitives_addon()){
		fprintf(stderr, "failed to initialize primitives!\n");
        return -1;
    }	
	
	// Inicializa o modulo de carregamento de imagens (ainda nao utilizado).
	if(!al_init_image_addon()){
		fprintf(stderr, "failed to initialize image module!\n");
		return -1;
	}
   
	// Cria um temporizador que gera eventos a cada 1.0/FPS segundos.
    timer = al_create_timer(1.0 / FPS);
    if(!timer) {
		fprintf(stderr, "failed to create timer!\n");
		return -1;
	}
 
	// Cria uma janela de SCREEN_W por SCREEN_H pixels.
	display = al_create_display(SCREEN_W, SCREEN_H);
	if(!display) {
		fprintf(stderr, "failed to create display!\n");
		al_destroy_timer(timer);
		return -1;
	}

	// Habilita a entrada por teclado.
	if(!al_install_keyboard()) {
		fprintf(stderr, "failed to install keyboard!\n");
		return -1;
	}
	

	// Inicializa o modulo de fontes da Allegro.
	al_init_font_addon();

	// Inicializa o modulo que permite carregar fontes TrueType (TTF).
	if(!al_init_ttf_addon()) {
		fprintf(stderr, "failed to load tff font module!\n");
		return -1;
	}
	
	// Carrega novamente uma fonte local de tamanho 32 para uso na tela final.
	initGlobalVars();
	if(size_32 == NULL) {
		fprintf(stderr, "font file does not exist or cannot be accessed!\n");
	}

 	// Cria a fila de eventos utilizada pelo laco principal.
	event_queue = al_create_event_queue();
	if(!event_queue) {
		fprintf(stderr, "failed to create event_queue!\n");
		al_destroy_display(display);
		return -1;
	}
   


	// Registra eventos da janela (por exemplo, o clique no botao de fechar).
	al_register_event_source(event_queue, al_get_display_event_source(display));
	// Registra os eventos produzidos pelo temporizador.
	al_register_event_source(event_queue, al_get_timer_event_source(timer));
	// Registra eventos de pressionamento e liberacao de teclas.
	al_register_event_source(event_queue, al_get_keyboard_event_source());

	Sub sub;
	initSub(&sub);

	Diver diver[NUM_DIVERS];
	
	for(i = 0; i < NUM_DIVERS; i++) {
		initDiver(&diver[i]);
	}

	// Inicia a emissao de eventos periodicos pelo temporizador.
	al_start_timer(timer);
	
	int playing = 1;
	while(playing) {
		ALLEGRO_EVENT ev;
		// Aguarda o proximo evento e armazena seus dados em ev.
		al_wait_for_event(event_queue, &ev);

		// A cada evento do temporizador, atualiza e desenha um quadro.
		if(ev.type == ALLEGRO_EVENT_TIMER) {

			updateGame(&sub, diver, &SCORE, &playing, &LIVES);
			drawGame(sub, diver);

			// Apresenta na janela o quadro que acabou de ser desenhado.
			al_flip_display();

			// Se o jogo terminou por colisao, mostra a situacao final por 2 segundos.
			if (!playing) {
				al_rest(2);
			}
		}
		// Fecha a partida caso a janela seja encerrada.
		else if(ev.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
			playing = 0;
		}

		// Ao pressionar uma tecla, altera a direcao de movimento.
		else if(ev.type == ALLEGRO_EVENT_KEY_DOWN) {

			switch(ev.keyboard.keycode) {
			// Tecla W: movimento para cima (no evento de pressionamento).
				case ALLEGRO_KEY_W:
					sub.dir_y--;
				break;

				case ALLEGRO_KEY_S:
					sub.dir_y++;
				break;

				case ALLEGRO_KEY_A:
					sub.dir_x--;
				break;

				case ALLEGRO_KEY_D:
					sub.dir_x++;
				break;	
			}
		}			

		else if(ev.type == ALLEGRO_EVENT_KEY_UP) {
			// Linha auxiliar de depuracao (desabilitada).
			//printf("\ncodigo tecla: %d", ev.keyboard.keycode);

			switch(ev.keyboard.keycode) {
			// Tecla W: interrompe a contribuicao da tecla para o movimento.
				case ALLEGRO_KEY_W:
					sub.dir_y++;
				break;

				case ALLEGRO_KEY_S:
					sub.dir_y--;
				break;

				case ALLEGRO_KEY_A:
					sub.dir_x++;
				break;

				case ALLEGRO_KEY_D:
					sub.dir_x--;
				break;	
			}
		}			

	} // Fim do laco principal.


	// Secao responsavel pela manipulacao do arquivo
	int record = 0;
	int bateu_record = 0;

	FILE *arq = fopen("record.txt", "r");
	if (arq != NULL) {
		if (fscanf(arq, "%d", &record) != 1) {
			record = 0;
		}
		fclose(arq);
	}

	if (SCORE > record) {
		bateu_record = 1;
		record = SCORE;

		FILE *arq = fopen("record.txt", "w");
		if (arq == NULL) {
			perror("ERRO! Falha ao escrever no arquivo");
		} else {
			fprintf(arq, "%d", record);
			fclose(arq);
		}
	}

	char linha_principal[50];
	char linha_secundaria[50];

	al_clear_to_color(al_map_rgb(230, 240, 250));

	if (bateu_record) {
		snprintf(linha_principal, sizeof(linha_principal), "NEW RECORD!");
		snprintf(linha_secundaria, sizeof(linha_secundaria), "Score: %d", SCORE);
	} else {
		snprintf(linha_principal, sizeof(linha_principal), "Score: %d", SCORE);
		snprintf(linha_secundaria, sizeof(linha_secundaria), "Record: %d", record);
	}

	al_draw_text(size_32, al_map_rgb(200, 0, 30), SCREEN_W / 3, SCREEN_H / 2 - 25, 0, linha_principal);
	al_draw_text(size_32, al_map_rgb(40, 40, 40), SCREEN_W / 3, SCREEN_H / 2 + 20, 0, linha_secundaria);

	

	// Apresenta a tela final na janela.
	al_flip_display();	
    al_rest(3);	
	
 
	if (size_32) {
		al_destroy_font(size_32);
	}
	al_destroy_timer(timer);
	al_destroy_display(display);
	al_destroy_event_queue(event_queue);
   
 
	return 0;
}