/**********************************************************************
 *
 * mqmmain.cpp - standalone MQM edition
 *
 * copyright (c) 2009 Ritsert Jansen, Danny Arends, Pjotr Prins and Karl W Broman
 *
 * last modified Apr,2009
 * first written Feb, 2009
 *
 *     This program is free software; you can redistribute it and/or
 *     modify it under the terms of the GNU General Public License,
 *     version 3, as published by the Free Software Foundation.
 *
 *     This program is distributed in the hope that it will be useful,
 *     but without any warranty; without even the implied warranty of
 *     merchantability or fitness for a particular purpose.  See the GNU
 *     General Public License, version 3, for more details.
 *
 *     A copy of the GNU General Public License, version 3, is available
 *     at http://www.r-project.org/Licenses/GPL-3
 *
 * C functions for the R/qtl package
 * Contains: R_scanMQM, scanMQM
 *
 **********************************************************************/

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <iostream>
#include <fstream>
#include "mqm.h"

using namespace std;

#ifdef STANDALONE

bool checkfileexists(const char *filename){
	ifstream myfile;
	bool exists;
	myfile.open(filename);
	exists = myfile.is_open();
	myfile.close();
	return exists;
}

struct algorithmsettings{
	unsigned int nind;
	unsigned int nmark;
	unsigned int npheno;
	int stepmin;
	int stepmax;
	unsigned int stepsize;
	unsigned int windowsize;
	double alpha;
	unsigned int maxiter;
	char estmap;
};

struct markersinformation{
	ivector markerchr;
	vector markerdistance;
	ivector markerparent;
};

struct algorithmsettings loadmqmsetting(const char* filename, bool verboseflag){
	algorithmsettings runsettings;
	if (verboseflag) printf("INFO: Loading settings from file\n");
	ifstream settingsstream(filename, ios::in);
	settingsstream >> runsettings.nind;
	settingsstream >> runsettings.nmark; //NEW we dont want to guess this: Should be Added to testfile
	settingsstream >> runsettings.npheno;
	settingsstream >> runsettings.stepmin;
	settingsstream >> runsettings.stepmax;
	settingsstream >> runsettings.stepsize;
	settingsstream >> runsettings.windowsize;
	settingsstream >> runsettings.alpha;
	settingsstream >> runsettings.maxiter;
	settingsstream >> runsettings.estmap;
	if (verboseflag) {
	    Rprintf("number of individuals: %d\n",runsettings.nind);
		Rprintf("number of markers: %d\n",runsettings.nmark);
	    Rprintf("number of phenotypes: %d\n",runsettings.npheno);
	    Rprintf("stepmin: %d\n",runsettings.stepmin);
	    Rprintf("stepmax: %d\n",runsettings.stepmax);
	    Rprintf("stepsize: %d\n",runsettings.stepsize);
	    Rprintf("windowsize for dropping qtls: %d\n",runsettings.windowsize);
	    Rprintf("Alpha level considered to be significant: %f\n",runsettings.alpha);
	    Rprintf("Max iterations using EM: %d\n",runsettings.maxiter);
		Rprintf("Re-estimating map-positions: %c\n",runsettings.estmap);
	}
	return runsettings;
}


cmatrix readgenotype(const char* filename,const unsigned int nind,const unsigned int nmar,const bool verboseflag){
	unsigned int cmarker = 0;
	unsigned int cindividual = 0;
	cmatrix genomarkers = newcmatrix(nmar,nind);
	ifstream myfstream(filename, ios::in);
	while (!myfstream.eof()) {
		if (cmarker < nmar){
			myfstream >> genomarkers[cmarker][cindividual];
			cmarker++;
		}else{
			cmarker = 0;
			cindividual++;
		}
    }
	if (verboseflag) Rprintf("Individuals: %d\n",cindividual);
	myfstream.close();
	return genomarkers;
}

matrix readphenotype(const char* filename,const unsigned int nind,const unsigned int nphe,const bool verboseflag){
	unsigned int cphenotype = 0;
	unsigned int cindividual = 0;
	matrix phenovalues = newmatrix(nphe,nind);
	ifstream myfstream(filename, ios::in);
	while (!myfstream.eof()) {
		if (cphenotype < nphe){
			myfstream >> phenovalues[cphenotype][cindividual];
			cphenotype++;
		}else{
			cphenotype = 0;
			cindividual++;
		}
    }
	if (verboseflag) Rprintf("Individuals: %d\n",cindividual);
	myfstream.close();
	return phenovalues;
}

struct markersinformation readmarkerfile(const char* filename,const unsigned int nmar,const bool verboseflag){
	unsigned int cmarker = 0;
	markersinformation info;
    ivector markerchr = newivector(nmar);			//NEW !!! chr-> should be added to test
	vector markerdistance= newvector(nmar);			//pos
	std::string markernames[nmar];					//NEW !!!
	ivector markerparent = newivector(nmar);		//f1genotype
	ifstream myfstream(filename, ios::in);
	while (!myfstream.eof()) {
		myfstream >> markerchr[cmarker];
		myfstream >> markernames[cmarker];
		myfstream >> markerdistance[cmarker];
		markerparent[cmarker] = 12;
		if (verboseflag) Rprintf("marker: %s %d %f\n",markernames[cmarker].c_str(),markerchr[cmarker],markerdistance[cmarker]);
		cmarker++;
	}
	if (verboseflag) Rprintf("Markers: %d\n",cmarker);
	//TODO get arrays back to main
	myfstream.close();
	info.markerchr=markerchr;
	info.markerdistance=markerdistance;
	info.markerparent=markerparent;
	return info;
}

unsigned int readcofactorfile(const char* filename,const unsigned int nmar,const bool verboseflag){
	if(checkfileexists(filename)){	//Because its an optional file
		unsigned int cmarker = 0;
		unsigned int set_cofactors = 0;
	    cvector cofactors = newcvector(nmar);
		ifstream myfstream(filename, ios::in);
		while (!myfstream.eof()) {
			myfstream >> cofactors[cmarker];
			if(cofactors[cmarker]!='0') set_cofactors++;
			cmarker++;
		}
		myfstream.close();
		if (verboseflag) Rprintf("Cofactors/Markers: %d/%d\n",set_cofactors,cmarker);
		return set_cofactors;
	}else{
		return 0;
	}
	//TODO get array back to main	
}

void printoptionshelp(void){
	printf ("Commandline switches:\n");
	printf ("-h      		This help.\n");
	printf ("-v      		Verbose (produce a lot of textoutput).\n");
	printf ("-p(INT) 		DebugLevel -d0,-d1.\n");
	printf ("-t(INT) 		Phenotype under analysis.\n");
	printf ("-p(FILE_NAME)	Phenotypes file in plain textformat.\n");
	printf ("-g(FILE_NAME)	Genotypes file in plain textformat.\n");
	printf ("-m(FILE_NAME)	Marker and Chromosome descriptionfile in plain textformat.\n");
	printf ("-s(FILE_NAME)	Settings file in plain textformat.\n");
	printf ("-c(FILE_NAME)	Optional Cofactors file  to do backward elimination on in plain textformat.\n");
 }

//Functions
void exitonerror(const char *msg){
	fprintf(stderr, msg);
	printoptionshelp();
	exit(1);
}
 



int main(int argc,char *argv[]) {
	Rprintf("MQM standalone version\n");
	bool verboseflag = false;
	bool helpflag = false;
	int debuglevel = 0;
	unsigned int phenotype = 0; //analyse the first phenotype
	char *phenofile = NULL;
	char *genofile = NULL;
	char *markerfile = NULL;
	char *coffile = NULL;
	char *settingsfile = NULL; 
	struct algorithmsettings mqmalgorithmsettings;
	struct markersinformation mqmmarkersinfo;
	unsigned int index;
	signed int c;
	//Parsing of arguments     
	while ((c = getopt (argc, argv, "vd:h:p:g:m:c:s:t:")) != -1)
	switch (c)
	{
		case 'v':
			verboseflag = true;
		break;
		case 'h':
			helpflag = true;
		break;	
		case 'd': 
			debuglevel = atoi(optarg);
		break;
		case 't':						//1 phenotype at a time
			phenotype = atoi(optarg);
		break;
		case 'p':
			phenofile = optarg;
		break;
		case 'g':
			genofile = optarg;
		break;             
		case 'm':
			markerfile = optarg;
		break;
		case 's':
			settingsfile = optarg;
		break;	
		case 'c':
			coffile = optarg;
		break;
		default:
			fprintf (stderr, "Unknown option character '%c'.\n", optopt);
	}
	if(helpflag){
		printoptionshelp();
		return 0;
	}else{
		printf ("Options for MQM:\n");
	//Verbose & debug
		printf ("verboseflag = %d, debuglevel = %d\n",verboseflag, debuglevel);
	//Needed files
		if(!phenofile) exitonerror("Please supply a phenotypefile argument.\n");
		if(!checkfileexists(phenofile)) exitonerror("Phenotypefile not found on your filesystem.\n");
		printf ("Phenotypefile = %s\n",phenofile);
		if(!genofile)  exitonerror("Please supply a genofile argument.\n");
		if(!checkfileexists(genofile)) exitonerror("Genotypefile not found on your filesystem.\n");
		printf ("Genotypefile = %s\n",genofile);
		if(!markerfile) exitonerror("Please supply a markerfile argument.\n");
		if(!checkfileexists(genofile)) exitonerror("Markerfile not found on your filesystem.\n");
		printf ("Markerfile = %s\n",markerfile);
		if(!settingsfile) exitonerror("Please supply a settingsfile argument.\n");
		if(!checkfileexists(settingsfile)) exitonerror("settingsfile not found on your filesystem.\n");
		printf ("settingsfile = %s\n",settingsfile);	
	//Optional files
		if(!coffile){
			if(!checkfileexists(coffile)){
				printf("Cofactorfile not found on your filesystem.\n");
			}else{
				printf ("Cofactorfile = %s\n",coffile);
			}
		}
	//Warn people for non-existing options
		for (index = optind; index < argc; index++){
			printf ("Non-option argument %s\n", argv[index]);
		}
	//Read in settingsfile
		mqmalgorithmsettings = loadmqmsetting(settingsfile,verboseflag);
	//Create large datastructures	
		double **QTL;
		ivector f1genotype = newivector(mqmalgorithmsettings.nmark);
		ivector chr = newivector(mqmalgorithmsettings.nmark);
		cvector cofactor = newcvector(mqmalgorithmsettings.nmark);
		vector mapdistance = newvector(mqmalgorithmsettings.nmark);
		vector pos = newvector(mqmalgorithmsettings.nmark);
		matrix pheno_value = newmatrix(mqmalgorithmsettings.npheno,mqmalgorithmsettings.nind);
		cmatrix markers= newcmatrix(mqmalgorithmsettings.nmark,mqmalgorithmsettings.nind);
		ivector INDlist= newivector(mqmalgorithmsettings.nind);
	//Some additional variables
		int set_cofactors=0;			//Markers set as cofactors
		int backwards=0;				//Backward elimination ?
		MQMCrossType crosstype = CF2;	//Crosstype
		
	//Here we know what we need so we can start reading in files with the new loader functions
		markers = readgenotype(genofile,mqmalgorithmsettings.nind,mqmalgorithmsettings.nmark,verboseflag);
		
		if (verboseflag) Rprintf("Genotypefile done\n");
		
		pheno_value = readphenotype(phenofile,mqmalgorithmsettings.nind,mqmalgorithmsettings.npheno,verboseflag);
  
		if (verboseflag) Rprintf("Phenotypefile done \n");

		mqmmarkersinfo = readmarkerfile(markerfile,mqmalgorithmsettings.nmark,verboseflag);
		chr = mqmmarkersinfo.markerchr;
		pos = mqmmarkersinfo.markerdistance;
		f1genotype = mqmmarkersinfo.markerparent;

		if (verboseflag) Rprintf("Markerposition file done\n");

		set_cofactors = readcofactorfile(coffile,mqmalgorithmsettings.nmark,verboseflag);
		if(set_cofactors > 0){
			backwards = 1;
		}
		if (verboseflag) Rprintf("%d markers with cofactors backward elimination enabled\n",set_cofactors);
  
		//Determin how many chromosomes we have to enable output
		unsigned int max_chr=0;
		for (int m=0; m < mqmalgorithmsettings.nmark; m++) {
			if(max_chr<chr[m]){
				max_chr = chr[m];
			}
		}
		if (verboseflag)  Rprintf("# %d Chromosomes\n",max_chr);
  
		int locationsoutput = 3*max_chr*(((mqmalgorithmsettings.stepmax)-(mqmalgorithmsettings.stepmin))/ (mqmalgorithmsettings.stepsize));
		QTL = newmatrix(1,locationsoutput);

		for (int i=0; i< mqmalgorithmsettings.nmark; i++) {
			cofactor[i] = '0';
			mapdistance[i]=999.0;
			mapdistance[i]=pos[i];
		}
		for (int i=0; i< mqmalgorithmsettings.nind; i++) {
			INDlist[i] = i;
		}
		
		//<dataaugmentation>
		//  cmatrix new_markers;
		//  vector new_y;
		// ivector new_ind;
		//int nAug, Nmark = nMark;
		// int maxind = 1000;
		// int maxiaug = 8;
		// int neglect = 1;
		// cvector position = locate_markers(mqmalgorithmsettings.nmark,chr);
		// vector r = recombination_frequencies(mqmalgorithmsettings.nmark, position, mapdistance);
		// augmentdata(markers, pheno_value[phenotype], &new_markers, &new_y, &new_ind, &nInd, &nAug, Nmark, position, r, maxind, maxiaug, neglect, crosstype, verboseflag);
		// neglect = 3;
		// augmentdata(markers, pheno_value[phenotype], &new_markers, &new_y, &new_ind, &nInd, &nAug, Nmark, position, r, maxind, maxiaug, neglect, crosstype, verboseflag);
		// Output marker info
		// for (int m=0; m<mqmalgorithmsettings.nmark; m++) {
		//	Rprintf("%d\t%f\n",m,mapdistance[m]);
		// }
		// </dataaugmentation>
		
		// ignores augmented set, for now...
		analyseF2(mqmalgorithmsettings.nind, mqmalgorithmsettings.nmark, &cofactor, markers, pheno_value[phenotype], f1genotype, backwards,QTL, &mapdistance,&chr,0,0,mqmalgorithmsettings.windowsize,
				  mqmalgorithmsettings.stepsize,mqmalgorithmsettings.stepmin,mqmalgorithmsettings.stepmax,mqmalgorithmsettings.alpha,mqmalgorithmsettings.maxiter,mqmalgorithmsettings.nind,&INDlist,mqmalgorithmsettings.estmap,crosstype,0,verboseflag);
		
		// Output marker info
		//for (int m=0; m<mqmalgorithmsettings.nmark; m++) {
		//  Rprintf("%5d%3d%9.3f\n",m,chr[m],mapdistance[m]);
		//}
		// Output (augmented) QTL info
		for (int q=0; q<locationsoutput; q++) {
			Rprintf("%5d%10.5f\n",q,QTL[0][q]);
		}
		freevector((void *)f1genotype);
		freevector((void *)cofactor);
		freevector((void *)mapdistance);
		freematrix((void **)markers,mqmalgorithmsettings.nmark);
		freematrix((void **)pheno_value,mqmalgorithmsettings.npheno);
		freevector((void *)chr);
		freevector((void *)INDlist);
		freevector((void *)pos);
		freematrix((void **)QTL,1);
		return 0;
	}
}

#else
#error "Is this a STANDALONE version? STANDALONE should be defined in the build system!"
#endif