/**********************************************************************
 * $Id$
 *
 * GEOS - Geometry Engine Open Source
 * http://geos.refractions.net
 *
 * Copyright (C) 2001-2002 Vivid Solutions Inc.
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU Lesser General Public Licence as published
 * by the Free Software Foundation. 
 * See the COPYING file for more information.
 *
 **********************************************************************
 * $Log$
 * Revision 1.1  2004/04/08 04:53:56  ybychkov
 * "operation/polygonize" ported from JTS 1.4
 *
 *
 **********************************************************************/


#include "../../headers/opPolygonize.h"
#include <typeinfo>

namespace geos {

Polygonizer::LineStringAdder::LineStringAdder(Polygonizer *p) {
	pol=p;
}
void Polygonizer::LineStringAdder::filter_rw(Geometry *g) {
	if (typeid(*g)==typeid(LineString))
		pol->add((LineString*) g);
}


/**
* Create a polygonizer with the same {@link GeometryFactory}
* as the input {@link Geometry}s
*/
Polygonizer::Polygonizer() {
	lineStringAdder=new Polygonizer::LineStringAdder(this);
	graph=NULL;
	dangles=new vector<LineString*>();
	cutEdges=new vector<LineString*>();
	invalidRingLines=new vector<LineString*>();
	holeList=NULL;
	shellList=NULL;
	polyList=NULL;
}

Polygonizer::~Polygonizer() {
	delete lineStringAdder;
	delete dangles;
	delete cutEdges;
	delete invalidRingLines;
	delete graph;
//	holeList=NULL;
//	shellList=NULL;
//	polyList=NULL;
}

/**
* Add a collection of geometries to be polygonized.
* May be called multiple times.
* Any dimension of Geometry may be added;
* the constituent linework will be extracted and used
*
* @param geomList a list of {@link Geometry}s with linework to be polygonized
*/
void Polygonizer::add(vector<Geometry*> *geomList){
	for(int i=0;i<(int)geomList->size();i++) {
		Geometry *geometry=(*geomList)[i];
		add(geometry);
	}
}

/**
* Add a geometry to the linework to be polygonized.
* May be called multiple times.
* Any dimension of Geometry may be added;
* the constituent linework will be extracted and used
*
* @param g a {@link Geometry} with linework to be polygonized
*/
void Polygonizer::add(Geometry *g) {
	g->apply_rw(lineStringAdder);
}

/**
* Add a linestring to the graph of polygon edges.
*
* @param line the {@link LineString} to add
*/
void Polygonizer::add(LineString *line){
	// create a new graph using the factory from the input Geometry
	if (graph==NULL)
		graph=new PolygonizeGraph(line->getFactory());
	graph->addEdge(line);
}

/**
* Gets the list of polygons formed by the polygonization.
* @return a collection of {@link Polygons}
*/
vector<Polygon*>* Polygonizer::getPolygons(){
	polygonize();
	return polyList;
}

/**
* Get the list of dangling lines found during polygonization.
* @return a collection of the input {@LineStrings} which are dangles
*/
vector<LineString*>* Polygonizer::getDangles(){
	polygonize();
	return dangles;
}

/**
* Get the list of cut edges found during polygonization.
* @return a collection of the input {@LineStrings} which are cut edges
*/
vector<LineString*>* Polygonizer::getCutEdges() {
	polygonize();
	return cutEdges;
}

/**
* Get the list of lines forming invalid rings found during polygonization.
* @return a collection of the input {@LineStrings} which form invalid rings
*/
vector<LineString*>* Polygonizer::getInvalidRingLines(){
	polygonize();
	return invalidRingLines;
}

/**
* Perform the polygonization, if it has not already been carried out.
*/
void Polygonizer::polygonize() {
	// check if already computed
	if (polyList!=NULL) return;
	dangles=graph->deleteDangles();
	cutEdges=graph->deleteCutEdges();
	vector<polygonizeEdgeRing*> *edgeRingList=graph->getEdgeRings();
	vector<polygonizeEdgeRing*> *validEdgeRingList=new vector<polygonizeEdgeRing*>();
	invalidRingLines=new vector<LineString*>();
	findValidRings(edgeRingList, validEdgeRingList, invalidRingLines);
	findShellsAndHoles(validEdgeRingList);
	assignHolesToShells(holeList, shellList);
	polyList=new vector<Polygon*>();
	for (int i=0;i<(int)shellList->size();i++) {
		polygonizeEdgeRing *er=(*shellList)[i];
		polyList->push_back(er->getPolygon());
	}
}

void Polygonizer::findValidRings(vector<polygonizeEdgeRing*> *edgeRingList, vector<polygonizeEdgeRing*> *validEdgeRingList,vector<LineString*> *invalidRingList){
	for (int i=0;i<(int)edgeRingList->size();i++) {
		polygonizeEdgeRing *er=(*edgeRingList)[i];
		if (er->isValid())
			validEdgeRingList->push_back(er);
		else
			invalidRingList->push_back(er->getLineString());
	}
}

void Polygonizer::findShellsAndHoles(vector<polygonizeEdgeRing*> *edgeRingList) {
	holeList=new vector<polygonizeEdgeRing*>();
	shellList=new vector<polygonizeEdgeRing*>();
	for (int i=0;i<(int)edgeRingList->size();i++) {
		polygonizeEdgeRing *er=(*edgeRingList)[i];
		if (er->isHole())
			holeList->push_back(er);
		else
			shellList->push_back(er);
	}
}

void Polygonizer::assignHolesToShells(vector<polygonizeEdgeRing*> *holeList,vector<polygonizeEdgeRing*> *shellList) {
	for (int i=0;i<(int)holeList->size();i++) {
		polygonizeEdgeRing *holeER=(*holeList)[i];
		assignHoleToShell(holeER,shellList);
	}
}

void Polygonizer::assignHoleToShell(polygonizeEdgeRing *holeER,vector<polygonizeEdgeRing*> *shellList) {
	polygonizeEdgeRing *shell=polygonizeEdgeRing::findEdgeRingContaining(holeER, shellList);
	if (shell!=NULL)
		shell->addHole(holeER->getRing());
}


}
