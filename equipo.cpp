#include "equipo.h"


direccion Equipo::apuntar_a(coordenadas pos1, coordenadas pos2) {
	if (pos2.second > pos1.second) return ABAJO;
	if (pos2.second < pos1.second) return ARRIBA;
	return (pos2.first > pos1.first) ? DERECHA : IZQUIERDA;
}


void Equipo::jugador(int nro_jugador) {
	//
	// ...
	//

	while(!this->belcebu->termino_juego()) { // Chequear que no haya una race condition en gameMaster
		switch(this->strat) {
			//SECUENCIAL,RR,SHORTEST,USTEDES
			case(SECUENCIAL): {
				sem_t turno = (this->equipo == AZUL) ? (this->belcebu->turno_azul) : (this->belcebu->turno_rojo);
				sem_wait(&turno);
				// el que empiece libre, arranca. cuando vuelva a entrar, el lock lo va a frenar.
				disponible_jugada[nro_jugador].lock(); 
				
				if(quantum_restante > 0) {
					int random_pick = rand() % 4;
					direccion proxima_dir = vector<direccion>{ARRIBA, ABAJO, IZQUIERDA, DERECHA}[random_pick];
					printlock.lock();
					cout << "Moviendo el jugador: " << nro_jugador << " del equipo " << ((this->equipo == ROJO) ? ("ROJO") : ("AZUL")) <<  " hacia " << proxima_dir << endl;
					cout << "Quantum restante: " << quantum_restante << endl;
					cout << "----------------------------------" << endl;
					printlock.unlock();
					this->belcebu->mover_jugador(proxima_dir, nro_jugador);
					quantum_restante--;

					if(quantum_restante == 0) {
						quantum_restante = this->quantum;
						this->belcebu->termino_ronda(this->equipo);
					}

				}
				disponible_jugada[(nro_jugador + 1) % cant_jugadores /*esto define un orden, cambiar por random ?*/].unlock();
				break;
			}	
			case(RR):
				//
				// ...
				//
				break;

			case(SHORTEST):
				//
				// ...
				//
				break;

			case(USTEDES):
				//
				// ...
				//
				break;
			default:
				break;
		}	
		// Termino ronda ? Recordar llamar a belcebu...
		// OJO. Esto lo termina un jugador... 
		//
		// ...
		//
	}
	
}

Equipo::Equipo(gameMaster *belcebu, color equipo, 
		estrategia strat, int cant_jugadores, int quantum, vector<coordenadas> posiciones) {

	this->belcebu = belcebu;
	this->equipo = equipo;
	this->contrario = (equipo == ROJO)? AZUL : ROJO;
	this->bandera_contraria = (equipo==ROJO)? BANDERA_AZUL : BANDERA_ROJA;
	this->strat = strat;
	this->quantum = quantum;
	this->quantum_restante = quantum;
	this->cant_jugadores = cant_jugadores;
	this->posiciones = posiciones;
	this->disponible_jugada = vector<mutex>(cant_jugadores);
	//
	// ...
	//
	for(int i = 0; i < cant_jugadores; i++){
		if(i != 0) disponible_jugada[i].lock();
	}

	//Si necesitamos mas semaforos van aca
	if(equipo == AZUL){
		sem_init(&belcebu->turno_azul, 0, 0);
	} else {
		sem_init(&belcebu->turno_rojo, 0, 1);
	}

}

void Equipo::comenzar() {
	// Arranco cuando me toque el turno 
	// TODO: Quien empieza ? 
	//
	// ...
	//
	
	// Creamos los jugadores
	for(int i=0; i < cant_jugadores; i++) {
		jugadores.emplace_back(thread(&Equipo::jugador, this, i)); 
	}

}

void Equipo::terminar() {
	for(auto &t:jugadores){
		t.join();
	}	
}

coordenadas Equipo::buscar_bandera_contraria() {
	//
	// ...
	//
	return pair<int,int>{0,0};
}
