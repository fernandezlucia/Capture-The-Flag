#include <iostream>
#include <thread>
#include <sys/unistd.h>
#include <vector>
#include <mutex>

#include "gameMaster.h"
#include "equipo.h"
#include "definiciones.h"
#include "config.h"

using namespace std;

estrategia strat = SECUENCIAL;
int quantum = 10;
string filename = "../config/config_parameters_catedra.csv";

int main(int argc, char* argv[]){
    bool estrategia_desc = false;
    if ( argc >= 4 ) {
        string s = argv[1];
        
        if(s == "SECUENCIAL"){
            strat = SECUENCIAL;
        }
        else if(s == "RR"){
            strat = RR;
        }
        else if(s == "USTEDES"){
            strat = USTEDES;
        }
        else if(s == "SHORTEST"){
            strat = SHORTEST;
        }
        else {
            estrategia_desc = true;
            cout << FYEL("Estrategia desconocida, usando default (SECUENCIAL, RR, SHORTEST, USTEDES son validas).") << endl;
        }

        quantum = atoi(argv[2]);
        filename = argv[3];
    }

    string def = (estrategia_desc ? ("default") : (argv[1]));

    cout << "El quantum elegido fue : " 
    << quantum << " para el archivo en " 
    << filename << " con estrategia " 
    << def << endl;

    Config config = *(new Config(filename));
	
    gameMaster belcebu = gameMaster(config);

	// Creo equipos (lanza procesos)
	Equipo rojo(&belcebu, ROJO, strat, config.cantidad_jugadores, quantum, config.pos_rojo);
	Equipo azul(&belcebu, AZUL, strat, config.cantidad_jugadores, quantum, config.pos_azul);

    rojo.comenzar();
    azul.comenzar();
    rojo.terminar();
	azul.terminar();
    belcebu.play();

    cout << "Bandera capturada por el equipo "<< belcebu.ganador << ". Felicidades!" << endl;

}

