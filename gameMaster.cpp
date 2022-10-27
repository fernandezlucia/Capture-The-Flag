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

	// Seteo tablero vacio
	tablero.resize(x);
    for (int i = 0; i < x; ++i) {
        tablero[i].resize(y);
        fill(tablero[i].begin(), tablero[i].end(), VACIO);
    }

    // Spawn jugadores rojos
    for(auto &coord : config.pos_rojo){
        assert(es_color_libre(tablero[coord.first][coord.second])); //Compruebo que no haya otro jugador en esa posicion
        tablero[coord.first][coord.second] = ROJO; // guardo la posicion
    }

    // Spawn jugadores azules
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

int distancia_del_taxista(coordenadas p1, coordenadas p2){
    return abs((p1.first - p2.first) + (p1.second - p2.second));
}

bool gameMaster::soy_el_mas_cercano(int nro_jugador, color equipo){
    int masCercano = 0;
    if(equipo == ROJO){
        int distancia_de_jugador_actual = distancia_del_taxista(pos_jugadores_rojos[nro_jugador], pos_bandera_azul);
        for(int i = 0; i < pos_jugadores_rojos.size(); i++) {
            if(distancia_de_jugador_actual > distancia_del_taxista(pos_jugadores_rojos[i], pos_bandera_azul)) return false;            
        }
    }
    else {
        int distancia_de_jugador_actual = distancia_del_taxista(pos_jugadores_azules[nro_jugador], pos_bandera_roja);
        for(int i = 0; i < pos_jugadores_azules.size(); i++) {
            if(distancia_de_jugador_actual > distancia_del_taxista(pos_jugadores_azules[i], pos_bandera_roja)) return false;            
        }
    }
    return true;
}

int gameMaster::mover_jugador(direccion dir, int nro_jugador) {
	int res = nro_ronda;
	
	coordenadas posicionJugador;
	if(turno == AZUL) {
		posicionJugador = this->pos_jugadores_azules[nro_jugador];	
	} else {
        posicionJugador = this->pos_jugadores_rojos[nro_jugador];
    }
	coordenadas proximaPosicion = proxima_posicion(posicionJugador, dir);

    // Chequear que la movida sea valida (adentro del tablero):
    bool pos_valida = es_posicion_valida(proximaPosicion);
    if(!pos_valida) {
        cout << "Me intente mover a una posicion invalida entonces no hice nada, soy "
             << nro_jugador << " de "
             << ((turno == AZUL) ? ("AZUL") : ("ROJO"))
             << " en (" << proximaPosicion.first << "," << proximaPosicion.second << ")"
             << endl;
        return nro_ronda;
    }

	bool es_libre = es_color_libre(en_posicion(proximaPosicion)); //La proxima posicion es vacia en el tablero?
    color bandera_contraria = turno == AZUL ? BANDERA_ROJA : BANDERA_AZUL;
	bool bandera_objetivo = en_posicion(proximaPosicion) == bandera_contraria; //La proxima posicion es la bandera?

	// No me puedo mover, por el momento termino mi turno si a donde
	// apunto no puedo, pero habria que buscar alternativa, es decir, 
	// tener un listado de posibles movimientos para el jugador i, 
	// y probar hasta agotarlos. 

	if(!es_libre && !bandera_objetivo) {
		cout << "No me pude mover hacia ("
             << proximaPosicion.first << ", " << proximaPosicion.second
             << "), soy "
             << nro_jugador << " de "
		     << ((turno == AZUL) ? ("AZUL") : ("ROJO"))
             << ", Reintento: ";

        coordenadas* next = this->movimiento_alternativo(posicionJugador, dir, proximaPosicion);
        bool success = false;
        int i = 0;

        while(!success){
            es_libre = es_color_libre(en_posicion(next[i])); //La proxima posicion es vacia en el tablero?
            bandera_objetivo = en_posicion(next[i]) == bandera_contraria; //La proxima posicion es la bandera?
            if(!es_libre && !bandera_objetivo){
                if(i < 3){
                    i++;
                } else {
                    cout << "Fracasé :(" << endl;
                    return nro_ronda;
                }
            } else {
                cout << "Exito! :)   -->";
                proximaPosicion = next[i];
                break;
            }
        }
    }

    cout << "Realizando turno de: "
         << nro_jugador << " "
         << ((turno == AZUL) ? ("AZUL") : ("ROJO"))
         << " moviendose desde ("
         << posicionJugador.first
         << ","
         << posicionJugador.second
         << ") hacia ("
         << proximaPosicion.first
         << ","
         << proximaPosicion.second
         << "). Nro Ronda: " << nro_ronda << "."
         << endl;

    if(!termino_juego()) {

		if(turno == AZUL){
            //Actualizo posicion en estructura de belcebu
			this->pos_jugadores_azules[nro_jugador] = proximaPosicion;

            // Chequeamos si estamos en la bandera del otro equipo, de ser asi, ganamos.
            if(es_posicion_bandera(proximaPosicion, ROJO)){
                cout << "Gano el azul!!!" << endl;
				ganador = AZUL;
				res = 0;
			} else {
				mover_jugador_tablero(posicionJugador, proximaPosicion, AZUL);
			}

		} else {
            //Actualizo posicion en estructura de belcebu
            this->pos_jugadores_rojos[nro_jugador] = proximaPosicion;

            // Chequeamos si estamos en la bandera del otro equipo, de ser asi, ganamos.
			if(es_posicion_bandera(proximaPosicion, AZUL)){
                cout << "Gano el rojo!!!" << endl;
				ganador = ROJO;
				res = 0;
			} else {
				mover_jugador_tablero(posicionJugador, proximaPosicion, ROJO);
			}

		}
		
	} else {
		// SKIP
	}

	// TODO: actualizar coordenadas del equipo (decidir si lo hace el gamemaster o el equipo)
	return res;

}


void gameMaster::termino_ronda(color equipo) {
	// FIXME: Hacer chequeo de que es el color correcto que está llamando
	// FIXME: Hacer chequeo que hayan terminado todos los jugadores del equipo o su quantum (via mover_jugador)

	int i = 0;
	if(!termino_juego()){
		if(equipo == ROJO){
            for (int i = 0; i < this->jugadores_por_equipos; i++)
            {
                sem_post(&turno_azul);
            }
            turno = AZUL;				
		} else {
            for (int i = 0; i < this->jugadores_por_equipos; i++)
            {
                sem_post(&turno_rojo);
            }
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

//Idea: devolver un array de 3 elementos con la posicion mas sensata a intentar despues
//Considerando mas sensato un movimiento que no me aleje de la bandera
//Ej Si intentamos movernos arriba y no se pudo, intentamos derecha o izquiera, y de ultima opcion hacia abajo
//porque esta la ultima nos hace perder mas tiempo
coordenadas* gameMaster::movimiento_alternativo(coordenadas posicion, direccion intento_movimiento, coordenadas objetivo){
    coordenadas* res = new coordenadas[3];
    if(intento_movimiento == ARRIBA){

        if(objetivo.first < posicion.first){
            res[0] = this->proxima_posicion(posicion, IZQUIERDA);
            res[1] = this->proxima_posicion(posicion, DERECHA);
            res[2] = this->proxima_posicion(posicion, ABAJO);
        } else {
            res[0] = this->proxima_posicion(posicion, DERECHA);
            res[1] = this->proxima_posicion(posicion, IZQUIERDA);
            res[2] = this->proxima_posicion(posicion, ABAJO);
        }

    } else if(intento_movimiento == ABAJO){

        if(objetivo.first < posicion.first){
            res[0] = this->proxima_posicion(posicion, IZQUIERDA);
            res[1] = this->proxima_posicion(posicion, DERECHA);
            res[2] = this->proxima_posicion(posicion, ARRIBA);
        } else {
            res[0] = this->proxima_posicion(posicion, DERECHA);
            res[1] = this->proxima_posicion(posicion, IZQUIERDA);
            res[2] = this->proxima_posicion(posicion, ARRIBA);
        }

    } else if(intento_movimiento == IZQUIERDA){

        if(objetivo.second < posicion.second){
            res[0] = this->proxima_posicion(posicion, ARRIBA);
            res[1] = this->proxima_posicion(posicion, ABAJO);
            res[2] = this->proxima_posicion(posicion, DERECHA);
        } else {
            res[0] = this->proxima_posicion(posicion, ABAJO);
            res[1] = this->proxima_posicion(posicion, ARRIBA);
            res[2] = this->proxima_posicion(posicion, DERECHA);
        }

    } else {

        if(objetivo.second < posicion.second){
            res[0] = this->proxima_posicion(posicion, ARRIBA);
            res[1] = this->proxima_posicion(posicion, ABAJO);
            res[2] = this->proxima_posicion(posicion, IZQUIERDA);
        } else {
            res[0] = this->proxima_posicion(posicion, ABAJO);
            res[1] = this->proxima_posicion(posicion, ARRIBA);
            res[2] = this->proxima_posicion(posicion, IZQUIERDA);
        }
    }

    return res;
}


void gameMaster::play(){
	// cout << "Empezando 🤖, que gane el mejor..." << endl;
}