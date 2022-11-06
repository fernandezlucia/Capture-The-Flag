#include "equipo.h"
#include <random>
#define NAIVE_SEARCH true

direccion Equipo::apuntar_a(coordenadas pos1, coordenadas pos2) { 
	if (pos1.first > pos2.first) return IZQUIERDA;
	if (pos1.first < pos2.first) return DERECHA;
	// Estoy en la misma COL.
	if (pos1.second > pos2.second) return ARRIBA;
	if (pos1.second < pos2.second) return ABAJO;

	return ABAJO;
}


void Equipo::jugador(int nro_jugador) {
	
	//Busqueda de bandera
	coordenadas pos_actual;
	pos_actual.first = nro_jugador+1; 
	pos_actual.second = 1;

	while(!bandera_found && !hizo_naive_search){

		while(pos_actual.first <= this->belcebu->getTamx() && !bandera_found){

			if(this->belcebu->es_posicion_bandera(pos_actual, this->contrario)){
				bandera_found = true;
				clock_gettime(CLOCK_REALTIME, &tiempo_despues_threaded);
				cout << FMAG("Se tardo en encontrar la bandera ") 
					 << ((equipo == ROJO) ? FRED("ROJO") : FBLU("AZUL")) << " "
					 << tiempo_despues_threaded.tv_nsec - tiempo_antes_threaded.tv_nsec << " "
					 << FMAG("ns.") << endl;
				pos_bandera_contraria = pos_actual;
			}

			pos_actual.second = pos_actual.second + 1;
			if(pos_actual.second > this->belcebu->getTamy()){
				pos_actual.second = 1;
				pos_actual.first += this->cant_jugadores;
			}

		}
	}
	
	

	//Estrategias
	bool soy_el_mas_cercano = false;
	if(this->strat == SHORTEST){
		soy_el_mas_cercano = belcebu->soy_el_mas_cercano(nro_jugador,this->equipo);
	}
	if(!soy_el_mas_cercano && this->strat == SHORTEST) return;
	while(!this->belcebu->termino_juego()) {
		switch(this->strat) {
			case(SECUENCIAL): {
				int finalizador = -1;
				if(this->equipo == AZUL) {
                    sem_wait(&belcebu->turno_azul);
                } else {
                    sem_wait(&belcebu->turno_rojo);
					if(belcebu->ronda_actual() > 0){ 
						sem_wait(&belcebu->ronda_anterior_finalizada);
						sem_post(&belcebu->ronda_anterior_finalizada);
					}
                }

                coordenadas coords_bandera = buscar_bandera_contraria();
                direccion proxima_dir = apuntar_a(posiciones[nro_jugador], coords_bandera);

                //parte critica: moverse y esperar barrera
                moverse.lock();
				if(!this->belcebu->termino_juego()){
					belcebu->mover_jugador(proxima_dir, nro_jugador);
				}
                cant_jugadores_que_ya_jugaron++;
                if(cant_jugadores_que_ya_jugaron == cant_jugadores){
					finalizador = nro_jugador;
					if(belcebu->ronda_actual() > 0 && this->equipo == ROJO) sem_post(&belcebu->ronda_anterior_finalizada);
                }
                moverse.unlock();
                //fin parte critica
				
				if(cant_jugadores_que_ya_jugaron == cant_jugadores && finalizador == nro_jugador) {
					cant_jugadores_que_ya_jugaron = 0;
					this->belcebu->termino_ronda(this->equipo);
					for (int i = 0; i < cant_jugadores; i++) sem_post(&barrier);
				}

				sem_wait(&barrier);
				if(this->equipo == AZUL) {
					if(todos_terminaron.fetch_add(1) >= cant_jugadores - 1){
						todos_terminaron.fetch_sub(cant_jugadores);
						sem_post(&belcebu->ronda_anterior_finalizada);
					}
					sem_post(&belcebu->turno_rojo);
                } else {
                    sem_post(&belcebu->turno_azul);
                }

				break;
			}	
			case(RR): {
				int finalizador = -1;
				if(equipo == ROJO) sem_wait(&belcebu->mutexes_rr_rojos[nro_jugador]);
				else sem_wait(&belcebu->mutexes_rr_azules[nro_jugador]);

 				if(quantum_restante > 0) {
					coordenadas coords_bandera = buscar_bandera_contraria(); // ejecutar en la creacion del equipo
                	direccion proxima_dir = apuntar_a(posiciones[nro_jugador], coords_bandera);
					belcebu->mover_jugador(proxima_dir, nro_jugador); // sin mutex, sabemos que esta bloqueado
					quantum_restante--;
					if(this->equipo == AZUL) {
						sem_post(&belcebu->mutexes_rr_azules[(nro_jugador + 1) % cant_jugadores]);
					}else {
						sem_post(&belcebu->mutexes_rr_rojos[(nro_jugador + 1) % cant_jugadores]);
					}
				} else {
					finalizador = nro_jugador;
					quantum_restante = quantum;
					this->belcebu->termino_ronda(this->equipo);
				}
				if(nro_jugador == finalizador) {
					if(this->equipo == AZUL){
						sem_post(&belcebu->mutexes_rr_rojos[0]);	
					} else {
						sem_post(&belcebu->mutexes_rr_azules[0]);
					}
				}
				break;
			}
			case(SHORTEST) : {

				if(this->equipo == AZUL) {
                    sem_wait(&belcebu->turno_azul);
                } else {
                    sem_wait(&belcebu->turno_rojo); 
                }

				if(belcebu->ronda_actual() == 0 && this->equipo == ROJO){
					// Nos sacamos los turnos que ya no necesitamos
					for (size_t i = 0; i < cant_jugadores - 1; i++) sem_wait(&belcebu->turno_rojo);
				}

				coordenadas coords_bandera = buscar_bandera_contraria(); 
                direccion proxima_dir = apuntar_a(posiciones[nro_jugador], coords_bandera);
				belcebu->mover_jugador(proxima_dir, nro_jugador);
				this->belcebu->termino_ronda(this->equipo);

				if(this->equipo == AZUL) {
                    sem_post(&belcebu->turno_rojo);	
                } else { 
                	sem_post(&belcebu->turno_azul);
                }

				break;
			}
			case(USTEDES): {
				this_thread::sleep_for(100ms);
				if(equipo == ROJO) sem_wait(&belcebu->mutexes_rr_rojos[nro_jugador]);
				else sem_wait(&belcebu->mutexes_rr_azules[nro_jugador]);

 				while(quantums_por_jugador[nro_jugador] > 0) {
					coordenadas coords_bandera = buscar_bandera_contraria(); // ejecutar en la creacion del equipo
                	direccion proxima_dir = apuntar_a(posiciones[nro_jugador], coords_bandera);
					belcebu->mover_jugador(proxima_dir, nro_jugador); // sin mutex, sabemos que esta bloqueado
					quantums_por_jugador[nro_jugador]--;
				} 

				if(nro_jugador == cant_jugadores-1) {
					this->belcebu->termino_ronda(this->equipo);
					reiniciar_quantums();
					if(this->equipo == AZUL){
						sem_post(&belcebu->mutexes_rr_rojos[0]);	
					} else {
						sem_post(&belcebu->mutexes_rr_azules[0]);
					}
				} else {
					if(this->equipo == AZUL) {
						sem_post(&belcebu->mutexes_rr_azules[(nro_jugador + 1) % cant_jugadores]);
					} else {
						sem_post(&belcebu->mutexes_rr_rojos[(nro_jugador + 1) % cant_jugadores]);
					}
				}

				break;
			}
			default:
				break;
		}


		posiciones[nro_jugador] = belcebu->posicion_de(nro_jugador, this->equipo);


	}
	switch(this->strat){
		case(SECUENCIAL): {
			sem_post(&belcebu->ronda_anterior_finalizada);
			sem_post(&belcebu->turno_rojo);
			sem_post(&barrier);
			break;
		}
		case(RR): {
			if(this->equipo == AZUL){
				sem_post(&belcebu->mutexes_rr_rojos[(nro_jugador + 1) % cant_jugadores]);	
			} else {
				sem_post(&belcebu->mutexes_rr_azules[(nro_jugador + 1) % cant_jugadores]);
			}
			break;
		}
		default: {

		}
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
	this->quantums_por_jugador = vector<int>(cant_jugadores,0);

	for(int i = 0; i < cant_jugadores; i++){
		sem_t semaphore;
		if(equipo == ROJO) belcebu->mutexes_rr_rojos.emplace_back(semaphore);
		else belcebu->mutexes_rr_azules.emplace_back(semaphore);
		if(i == 0){
			if(equipo == ROJO) sem_init(&(belcebu->mutexes_rr_rojos[i]), 0, 1);
			else sem_init(&(belcebu->mutexes_rr_azules[i]), 0, 0);
		} else {
			if(equipo == ROJO) sem_init(&(belcebu->mutexes_rr_rojos[i]), 0, 0);
			else sem_init(&(belcebu->mutexes_rr_azules[i]), 0, 0);
		}
	}

	sem_init(&barrier, 0, 0);
	sem_init(&lejanos, 0, 0);
	
	for(int i = 0; i < cant_jugadores; i++){
		jugadores_ya_jugaron.push_back(0);
	}

	reiniciar_quantums();

	// interpretamos en la consigna que de no alcanzar para cubrir a todos los jugadores con el quantum,
	// todos se mueven una vez.
	if(quantum < cant_jugadores){
		quantum = cant_jugadores;
		quantum_restante = quantum;
	}

    //Iniciar su respectivo semaforo.
	if(equipo == AZUL){
		sem_init(&belcebu->turno_azul, 0, 0);
	} else {
		sem_init(&belcebu->turno_rojo, 0, cant_jugadores);
	}
}

void Equipo::reiniciar_quantums() {
	int maxQuantum = 10;
	for(int i = 0; i < cant_jugadores; i++){
		int random = rand() % maxQuantum;
		quantums_por_jugador[i] = random;
	}
}

void Equipo::comenzar() {
	
	if(NAIVE_SEARCH){
		hizo_naive_search = true;
		timespec tiempo_antes;
		timespec tiempo_despues;
		clock_gettime(CLOCK_REALTIME, &tiempo_antes);
		buscar_bandera_naive();
		clock_gettime(CLOCK_REALTIME, &tiempo_despues);
		cout << FMAG("Se tardo en encontrar la bandera (naive) ") 
					 << ((equipo == ROJO) ? FRED("ROJO") : FBLU("AZUL")) << " "
					 << tiempo_despues.tv_nsec - tiempo_antes.tv_nsec << " "
					 << FMAG("ns.") << endl;
	}
	
	if(!NAIVE_SEARCH) clock_gettime(CLOCK_REALTIME, &tiempo_antes_threaded);
	// Creamos los jugadores
	for(int i=0; i < cant_jugadores; i++) {
		
		jugadores.emplace_back(thread(&Equipo::jugador, this, i)); 
	}

}

void Equipo::terminar() {
	for(auto &t : jugadores){
		t.join();
	}
}

coordenadas Equipo::buscar_bandera_contraria() {
	return pos_bandera_contraria;
}

void Equipo::buscar_bandera_naive(){
	
	//mientras exista bandera contraria, esto la encuentra.
	for (size_t x = 1; x < belcebu->getTamx(); x++) {
		
		for (size_t y = 1; y < belcebu->getTamy(); y++) {
			coordenadas temp;
			temp.first = x;
			temp.second = y;
		
			if( belcebu->es_posicion_bandera(temp, this->contrario) ){
				bandera_found = true;
				pos_bandera_contraria = temp;
				return;
			}
		}
	}

}
