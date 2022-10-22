#include <sys/unistd.h>
#include <assert.h>     /* assert */
#include "gameMaster.h"

bool gameMaster::es_posicion_valida(coordenadas pos) {
	return  (pos.first > 0) &&
			(pos.first < x) && 
			(pos.second > 0) && 
			(pos.second < y);
}

bool gameMaster::es_color_libre(color color_tablero){
    return color_tablero == VACIO || color_tablero == INDEFINIDO;
}

color gameMaster::en_posicion(coordenadas coord) {
	return tablero[coord.first][coord.second];
}

int gameMaster::getTamx() {
	return x;
}

int gameMaster::getTamy() {
	return y;
}

int gameMaster::distancia(coordenadas c1, coordenadas c2) {
    return abs(c1.first-c2.first)+abs(c1.second-c2.second);
}

gameMaster::gameMaster(Config config) {
	assert(config.x>0); 
	assert(config.y>0); // Tamaño adecuado del tablero
	
    this->x = config.x;
	this->y = config.y;
	assert((config.bandera_roja.first == 1)); // Bandera roja en la primera columna
	assert(es_posicion_valida(config.bandera_roja)); // Bandera roja en algún lugar razonable

	assert((config.bandera_azul.first == x-1)); // Bandera azul en la primera columna
	assert(es_posicion_valida(config.bandera_azul)); // Bandera roja en algún lugar razonable

	assert(config.pos_rojo.size() == config.cantidad_jugadores);
	assert(config.pos_azul.size() == config.cantidad_jugadores);
	for(auto &coord : config.pos_rojo) {
		assert(es_posicion_valida(coord)); // Posiciones validas rojas
	}		

	for(auto &coord : config.pos_azul) {
		assert(es_posicion_valida(coord)); // Posiciones validas rojas
	}		

	
	this->jugadores_por_equipos = config.cantidad_jugadores;
	this->pos_bandera_roja = config.bandera_roja;
	this->pos_bandera_azul = config.bandera_azul;
    this->pos_jugadores_rojos = config.pos_rojo;
    this->pos_jugadores_azules = config.pos_azul;
	this->jugadasAzul = 0;
	this->jugadasRojo = 0;
	// Seteo tablero
	tablero.resize(x);
    for (int i = 0; i < x; ++i) {
        tablero[i].resize(y);
        fill(tablero[i].begin(), tablero[i].end(), VACIO);
    }

    for(auto &coord : config.pos_rojo){
        assert(es_color_libre(tablero[coord.first][coord.second])); //Compruebo que no haya otro jugador en esa posicion
        tablero[coord.first][coord.second] = ROJO; // guardo la posicion
    }

    for(auto &coord : config.pos_azul){
        assert(es_color_libre(tablero[coord.first][coord.second]));
        tablero[coord.first][coord.second] = AZUL;
    }

    tablero[config.bandera_roja.first][config.bandera_roja.second] = BANDERA_ROJA;
    tablero[config.bandera_azul.first][config.bandera_azul.second] = BANDERA_AZUL;
	this->turno = ROJO;

    cout << "SE HA INICIALIZADO GAMEMASTER CON EXITO" << endl;
    // Insertar código que crea necesario de inicialización 
}

void gameMaster::mover_jugador_tablero(coordenadas pos_anterior, coordenadas pos_nueva, color colorEquipo){
    assert(es_color_libre(tablero[pos_nueva.first][pos_nueva.second]));
    tablero[pos_anterior.first][pos_anterior.second] = VACIO; 
    tablero[pos_nueva.first][pos_nueva.second] = colorEquipo;
}


int gameMaster::mover_jugador(direccion dir, int nro_jugador) {
	// Chequear que la movida sea valida 						(adentro del tablero)
	int res = nro_ronda;
	
	coordenadas posicionJugador;
	if(turno == AZUL) {
		posicionJugador = this->pos_jugadores_azules[nro_jugador];	
	} else {
		posicionJugador = this->pos_jugadores_rojos[nro_jugador];	
	}
	coordenadas proximaPosicion = proxima_posicion(posicionJugador, dir);
	cout 
	<< "Moviendo al jugador " 
	<< nro_jugador 
	<< " a la posicion " 
	<< proximaPosicion.first 
	<< "," 
	<< proximaPosicion.second 
	<< endl;
	bool pos_valida = es_posicion_valida(proximaPosicion);
	assert(pos_valida); //in bounds

	/*
	una posicion no valida podria llegar a ser la bandera a la que vamos
	*/

	// Que no se puedan mover 2 jugadores a la vez
	mutexTurnos.lock();
	if(!termino_juego()) {

		if(turno == AZUL){
			this->pos_jugadores_azules[nro_jugador] = proximaPosicion;
			mover_jugador_tablero(posicionJugador, proximaPosicion, AZUL);

			if(es_posicion_bandera(proximaPosicion, ROJO)){
				ganador = AZUL;
				res = 0;
			}

		} else {
			this->pos_jugadores_rojos[nro_jugador] = proximaPosicion;
			mover_jugador_tablero(posicionJugador, proximaPosicion, ROJO);

			if(es_posicion_bandera(proximaPosicion, AZUL)){
				ganador = ROJO;
				res = 0;
			}

		}
		
	} else {
		// SKIP
	}

	// Chequeamos si estamos en la bandera del otro equipo, de ser asi, ganamos. 
	

    // setear la variable ganador
	
	mutexTurnos.unlock();
    // Devolver acorde a la descripción

	
	// TODO: actualizar coordenadas del equipo (decidir si lo hace el gamemaster o el equipo)
	return res;

}


void gameMaster::termino_ronda(color equipo) {
	// FIXME: Hacer chequeo de que es el color correcto que está llamando
	// FIXME: Hacer chequeo que hayan terminado todos los jugadores del equipo o su quantum (via mover_jugador)
	if(!termino_juego()){
		if(equipo == ROJO){
			sem_post(&turno_azul);
			turno = AZUL;			
		}else{
			sem_post(&turno_rojo);
			turno = ROJO;
		}
	}
	nro_ronda++;
}

bool gameMaster::termino_juego() {
	return ganador != INDEFINIDO;
}

coordenadas gameMaster::proxima_posicion(coordenadas anterior, direccion movimiento) {
	// Calcula la proxima posición a moverse (es una copia) 
	switch(movimiento) {
		case(ARRIBA):
			anterior.second--; 
			break;

		case(ABAJO):
			anterior.second++;
			break;

		case(IZQUIERDA):
			anterior.first--;
			break;

		case(DERECHA):
			anterior.first++;
			break;
	}
	return anterior; // está haciendo una copia por constructor
}

bool gameMaster::es_posicion_bandera(coordenadas coord, color bandera){
	if(coord == pos_bandera_roja && bandera == ROJO){
		return true;
	} else if(coord == pos_bandera_azul && bandera == AZUL){
		return false;
	}
	return false;
}

void gameMaster::play(){
	cout << "Empezando 🤖, que gane el mejor..." << endl;
}