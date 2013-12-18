/*
 * Author: sijiewang
 * Date: 2013-12-18
 * E-mail: lnmcc@hotmail.com
 * Site: lnmcc.net
*/

#include "dumpPkg.h"

int main(int argc, char **argv) {

	Film aFilm = new Film("/home/sijiewang/Videos/21over-tlr1_h1080p.mov");
	Cinema aCinema = new Cinema();
	aCinema->playFilm(aFilm);
}
