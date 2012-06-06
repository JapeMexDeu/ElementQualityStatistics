/*
 * element_quality_statistics.cpp
 *
 *  Created on: 17.04.2012
 *      Author: Martin Stepniewski
 */


#include "element_quality_statistics.h"
#include "common/util/table.h"


namespace ug
{



////////////////////////////////////////////////////////////////////////////////////////////
//	CollectAssociatedSides
////////////////////////////////////////////////////////////////////////////////////////////

///	Collects all edges (= 2) which exist in the given face and which share the given vertex.
/**	This method uses Grid::mark **/
UG_API
inline void CollectAssociatedSides(EdgeBase* sidesOut[2], Grid& grid, Face* f, VertexBase* vrt)
{
	//PROFILE_BEGIN(CollectAssociatedSides_VERTEX);
	sidesOut[0] = NULL;
	sidesOut[1] = NULL;

	grid.begin_marking();
	for(size_t i = 0; i < f->num_vertices(); ++i){
		grid.mark(f->vertex(i));
	}

	//vector<EdgeBase*> vNeighbourEdgesToVertex;
	//CollectAssociated(vNeighbourEdgesToVertex, grid, vrt, true);

	Grid::AssociatedEdgeIterator iterEnd = grid.associated_edges_end(vrt);
	//Grid::AssociatedEdgeIterator iterEnd = vNeighbourEdgesToVertex.end();
	for(Grid::AssociatedEdgeIterator iter = grid.associated_edges_begin(vrt); iter != iterEnd; ++iter)
	//for(Grid::AssociatedEdgeIterator iter = vNeighbourEdgesToVertex.begin(); iter != iterEnd; ++iter)
	{
		EdgeBase* e = *iter;
		if(grid.is_marked(e->vertex(0)) && grid.is_marked(e->vertex(1))){
			UG_ASSERT(	sidesOut[1] == NULL,
						"Only two edges may be adjacent to a vertex in a face element.");

			if(sidesOut[0] == NULL)
				sidesOut[0] = e;
			else
				sidesOut[1] = e;
		}
	}

	grid.end_marking();
	UG_ASSERT(	sidesOut[1] != NULL,
				"Exactly two edges should be adjacent to a vertex in a face element.")
}

///	Collects all faces (= 2) which exist in the given volume and which share the given edge.
/**	This method uses Grid::mark **/
UG_API
inline void CollectAssociatedSides(Face* sidesOut[2], Grid& grid, Volume* v, EdgeBase* e)
{
	//PROFILE_BEGIN(CollectAssociatedSides_EDGE);
	sidesOut[0] = NULL;
	sidesOut[1] = NULL;

	grid.begin_marking();

	for(size_t i = 0; i < v->num_vertices(); ++i)
		grid.mark(v->vertex(i));

	vector<Face*> vNeighbourFacesToEdge;
	CollectAssociated(vNeighbourFacesToEdge, grid, e, true);

	//Grid::AssociatedFaceIterator iterEnd = grid.associated_faces_end(e);
	Grid::AssociatedFaceIterator iterEnd = vNeighbourFacesToEdge.end();
	//for(Grid::AssociatedFaceIterator iter = grid.associated_faces_begin(e); iter != iterEnd; ++iter)
	for(Grid::AssociatedFaceIterator iter = vNeighbourFacesToEdge.begin(); iter != iterEnd; ++iter)
	{
		Face* f = *iter;

	//	check whether all vertices of f are marked
		bool allMarked = true;
		for(size_t i = 0; i < f->num_vertices(); ++i){
			if(!grid.is_marked(f->vertex(i))){
				allMarked = false;
				break;
			}
		}

		if(allMarked){
			if(FaceContains(f, e)){
				UG_ASSERT(	sidesOut[1] == NULL,
							"Only two faces may be adjacent to an edge in a volume element.")

				if(sidesOut[0] == NULL)
					sidesOut[0] = f;
				else
					sidesOut[1] = f;
			}
		}
	}

	grid.end_marking();

	UG_ASSERT(	sidesOut[1] != NULL,
				"Exactly two faces should be adjacent to an edge in a volume element.")
}






////////////////////////////////////////////////////////////////////////////////////////////
//	CalculateMinAngle
////////////////////////////////////////////////////////////////////////////////////////////

//	An unimplemented version, so that a compile error occurs if no overload exists.
template <class TElem, class TAAPosVRT>
number CalculateMinAngle(Grid& grid, TElem* elem, TAAPosVRT& aaPos);

//	Face (Triangles and Quadrilaterals)
template <class TAAPosVRT>
number CalculateMinAngle(Grid& grid, Face* f, TAAPosVRT& aaPos)
{
	//PROFILE_FUNC();
//	in the current implementation this method requires, that all edges
//	are created for all faces.
//TODO: improve this!
	if(!grid.option_is_enabled(GRIDOPT_AUTOGENERATE_SIDES))
	{
		LOG("WARNING: autoenabling GRIDOPT_AUTOGENERATE_SIDES in GetNeighbours(Face).\n");
		grid.enable_options(GRIDOPT_AUTOGENERATE_SIDES);
	}

//	Get type of vertex attachment in aaPos and define it as ValueType
	typedef typename TAAPosVRT::ValueType ValueType;

//	Initialization
	uint numFaceVrts = f->num_vertices();
	ValueType vNorm1, vNorm2;
	ValueType vDir1, vDir2;
	number minAngle = 180.0;
	number tmpAngle;
	EdgeBase* vNeighbourEdgesToVertex[2];

//	Iterate over all face vertices
	for(uint vrtIter = 0; vrtIter < numFaceVrts; ++vrtIter)
	{
		VertexBase* vrt = f->vertex(vrtIter);

	//	Get adjacent edges at the current vertex and calculate the angle between their normals
		CollectAssociatedSides(vNeighbourEdgesToVertex, grid, f, vrt);

	//	Calculate direction vectors of the current two adjacent edges
	//	!!!	Beware of the correct order of the vertices	to get correct angle value !!!
		VecSubtract(vDir1,
					aaPos[vNeighbourEdgesToVertex[0]->vertex(0)],
					aaPos[vNeighbourEdgesToVertex[0]->vertex(1)]);

		VecSubtract(vDir2,
					aaPos[vNeighbourEdgesToVertex[1]->vertex(1)],
					aaPos[vNeighbourEdgesToVertex[1]->vertex(0)]);

	//	Normalize
		VecNormalize(vDir1, vDir1);
		VecNormalize(vDir2, vDir2);

	//	Calculate current angle
		tmpAngle = acos(VecDot(vDir1, vDir2));

	//	Check for minimality
		if(tmpAngle < minAngle)
		{
			minAngle = tmpAngle;
		}
	}

//	Transform minAngle from RAD to DEG
	minAngle = 180/PI * minAngle;

	return minAngle;
}

//	Tetrahedron
template <class TAAPosVRT>
number CalculateMinAngle(Grid& grid, Tetrahedron* tet, TAAPosVRT& aaPos)
{
	return CalculateMinAngle(grid, static_cast<Volume*>(tet), aaPos);
}

//	Prism
template <class TAAPosVRT>
number CalculateMinAngle(Grid& grid, Prism* prism, TAAPosVRT& aaPos)
{
	return CalculateMinAngle(grid, static_cast<Volume*>(prism), aaPos);
}

//	Pyramid
template <class TAAPosVRT>
number CalculateMinAngle(Grid& grid, Pyramid* pyr, TAAPosVRT& aaPos)
{
	return CalculateMinAngle(grid, static_cast<Volume*>(pyr), aaPos);
}

//	Volume (For volume elements the minimum of dihedral and edge angle will be returned.)
template <class TAAPosVRT>
number CalculateMinAngle(Grid& grid, Volume* v, TAAPosVRT& aaPos)
{
	//PROFILE_FUNC();
	number minDihedral;
	number tmpMinEdgeAngle;
	number minEdgeAngle = 360;

//	Calculate the minimal dihedral
	minDihedral = CalculateMinDihedral(grid, v, aaPos);

//	Calculate the minimal edge angle
	for(uint i = 0; i < v->num_faces(); ++i)
	{
		tmpMinEdgeAngle = CalculateMinAngle(grid, grid.get_face(v, i), aaPos);

		if(tmpMinEdgeAngle < minEdgeAngle)
		{
			minEdgeAngle = tmpMinEdgeAngle;
		}
	}

//	return the minimum of minimal dihedral and minimal edge angle
	return min(minDihedral, minEdgeAngle);
}


////////////////////////////////////////////////////////////////////////////////////////////
//	CalculateMinDihedral
////////////////////////////////////////////////////////////////////////////////////////////

//	An unimplemented version, so that a compile error occurs if no overload exists.
template <class TElem, class TAAPosVRT>
number CalculateMinDihedral(Grid& grid, TElem* elem, TAAPosVRT& aaPos);

//	Tetrahedron
template <class TAAPosVRT>
number CalculateMinDihedral(Grid& grid, Tetrahedron* tet, TAAPosVRT& aaPos)
{
	return CalculateMinDihedral(grid, static_cast<Volume*>(tet), aaPos);
}

//	Prism
template <class TAAPosVRT>
number CalculateMinDihedral(Grid& grid, Prism* prism, TAAPosVRT& aaPos)
{
	return CalculateMinDihedral(grid, static_cast<Volume*>(prism), aaPos);
}

//	Pyramid
template <class TAAPosVRT>
number CalculateMinDihedral(Grid& grid, Pyramid* pyr, TAAPosVRT& aaPos)
{
	return CalculateMinDihedral(grid, static_cast<Volume*>(pyr), aaPos);
}

//	Volume
template <class TAAPosVRT>
number CalculateMinDihedral(Grid& grid, Volume* v, TAAPosVRT& aaPos)
{
	//PROFILE_FUNC();
//	in the current implementation this method requires, that all edges
//	are created for all faces.
//TODO: improve this!
	if(!grid.option_is_enabled(GRIDOPT_AUTOGENERATE_SIDES))
	{
		LOG("WARNING: autoenabling GRIDOPT_AUTOGENERATE_SIDES in GetNeighbours(Face).\n");
		grid.enable_options(GRIDOPT_AUTOGENERATE_SIDES);
	}

//	Initialization
	uint numElementEdges = v->num_edges();
	vector3 vNorm1, vNorm2;
	number minDihedral = 360.0;
	number tmpAngle;
	Face* vNeighbourFacesToEdge[2];

//	Iterate over all element edges
	for(uint eIter = 0; eIter < numElementEdges; ++eIter)
	{
		EdgeBase* e = grid.get_edge(v, eIter);

	//	Get adjacent faces at the current edge and calculate the angle between their normals
		CollectAssociatedSides(vNeighbourFacesToEdge, grid, v, e);

		CalculateNormal(vNorm1, vNeighbourFacesToEdge[0], aaPos);
		CalculateNormal(vNorm2, vNeighbourFacesToEdge[1], aaPos);

	/*	!!!	Beware of the correct direction normals to get correct angle value !!!
		INFO:	Angles of a regular tetrahedron:
				- "Tetrahedron (dihedral) angle x" 	= 109,471...deg
				- "Face-to-face angle y"			= 180deg - x = 70,52...deg

		--> Old version:
		//VecScale(vNorm1, vNorm1, -1);
		//tmpAngle = acos(VecDot(vNorm1, vNorm2));

		--> New version:
			(s.	"Qualit�ts-Metriken und Optimierung von Tetraeder-Netzen",
				 Seminararbeit von Johannes Ahlmann, Universit�t Karlsruhe)
	*/
		tmpAngle = acos(VecDot(vNorm1, vNorm2));
		tmpAngle = PI - tmpAngle;

	//	Check for minimality
		if(tmpAngle < minDihedral)
		{
			minDihedral = tmpAngle;
		}
	}

//	Transform minAngle from RAD to DEG
	minDihedral = 180/PI * minDihedral;

	return minDihedral;
}


////////////////////////////////////////////////////////////////////////////////////////////
//	CalculateMaxAngle
////////////////////////////////////////////////////////////////////////////////////////////

//	An unimplemented version, so that a compile error occurs if no overload exists.
template <class TElem, class TAAPosVRT>
number CalculateMaxAngle(Grid& grid, TElem* elem, TAAPosVRT& aaPos);

//	Face (Triangles and Quadrilaterals)
template <class TAAPosVRT>
number CalculateMaxAngle(Grid& grid, Face* f, TAAPosVRT& aaPos)
{
	//PROFILE_FUNC();
//	in the current implementation this method requires, that all edges
//	are created for all faces.
//TODO: improve this!
	if(!grid.option_is_enabled(GRIDOPT_AUTOGENERATE_SIDES))
	{
		LOG("WARNING: autoenabling GRIDOPT_AUTOGENERATE_SIDES in GetNeighbours(Face).\n");
		grid.enable_options(GRIDOPT_AUTOGENERATE_SIDES);
	}

//	Get type of vertex attachment in aaPos and define it as ValueType
	typedef typename TAAPosVRT::ValueType ValueType;

//	Initialization
	uint numFaceVrts = f->num_vertices();
	ValueType vNorm1, vNorm2;
	ValueType vDir1, vDir2;
	number maxAngle = 0;
	number tmpAngle;
	EdgeBase* vNeighbourEdgesToVertex[2];

//	Iterate over all face vertices
	for(uint vrtIter = 0; vrtIter < numFaceVrts; ++vrtIter)
	{
		VertexBase* vrt = f->vertex(vrtIter);

	//	Get adjacent edges at the current vertex and calculate the angle between their normals
		CollectAssociatedSides(vNeighbourEdgesToVertex, grid, f, vrt);

	//	Calculate direction vectors of the current two adjacent edges
	//	!!!	Beware of the correct order of the vertices	to get correct angle value !!!
		VecSubtract(vDir1,
					aaPos[vNeighbourEdgesToVertex[0]->vertex(0)],
					aaPos[vNeighbourEdgesToVertex[0]->vertex(1)]);

		VecSubtract(vDir2,
					aaPos[vNeighbourEdgesToVertex[1]->vertex(1)],
					aaPos[vNeighbourEdgesToVertex[1]->vertex(0)]);

	//	Normalize
		VecNormalize(vDir1, vDir1);
		VecNormalize(vDir2, vDir2);

	//	Calculate current angle
		tmpAngle = acos(VecDot(vDir1, vDir2));

	//	Check for maximality
		if(tmpAngle > maxAngle)
		{
			maxAngle = tmpAngle;
		}
	}

//	Transform maxAngle from RAD to DEG
	maxAngle = 180/PI * maxAngle;

	return maxAngle;
}

//	Tetrahedron
template <class TAAPosVRT>
number CalculateMaxAngle(Grid& grid, Tetrahedron* tet, TAAPosVRT& aaPos)
{
	return CalculateMaxAngle(grid, static_cast<Volume*>(tet), aaPos);
}

//	Prism
template <class TAAPosVRT>
number CalculateMaxAngle(Grid& grid, Prism* prism, TAAPosVRT& aaPos)
{
	return CalculateMaxAngle(grid, static_cast<Volume*>(prism), aaPos);
}

//	Pyramid
template <class TAAPosVRT>
number CalculateMaxAngle(Grid& grid, Pyramid* pyr, TAAPosVRT& aaPos)
{
	return CalculateMaxAngle(grid, static_cast<Volume*>(pyr), aaPos);
}

//	Volume (For volume elements the maximum of dihedral and edge angle will be returned.)
template <class TAAPosVRT>
number CalculateMaxAngle(Grid& grid, Volume* v, TAAPosVRT& aaPos)
{
	//PROFILE_FUNC();
	number maxDihedral;
	number tmpMaxEdgeAngle;
	number maxEdgeAngle = 0;

//	Calculate the maximal dihedral
	maxDihedral = CalculateMaxDihedral(grid, v, aaPos);

//	Calculate the maximal edge angle
	for(uint i = 0; i < v->num_faces(); ++i)
	{
		tmpMaxEdgeAngle = CalculateMaxAngle(grid, grid.get_face(v, i), aaPos);

		if(tmpMaxEdgeAngle > maxEdgeAngle)
		{
			maxEdgeAngle = tmpMaxEdgeAngle;
		}
	}

//	return the maximum of maximal dihedral and maximal edge angle
	return max(maxDihedral, maxEdgeAngle);
}


////////////////////////////////////////////////////////////////////////////////////////////
//	CalculateMaxDihedral
////////////////////////////////////////////////////////////////////////////////////////////

//	An unimplemented version, so that a compile error occurs if no overload exists.
template <class TElem, class TAAPosVRT>
number CalculateMaxDihedral(Grid& grid, TElem* elem, TAAPosVRT& aaPos);

//	Tetrahedron
template <class TAAPosVRT>
number CalculateMaxDihedral(Grid& grid, Tetrahedron* tet, TAAPosVRT& aaPos)
{
	return CalculateMaxDihedral(grid, static_cast<Volume*>(tet), aaPos);
}

//	Prism
template <class TAAPosVRT>
number CalculateMaxDihedral(Grid& grid, Prism* prism, TAAPosVRT& aaPos)
{
	return CalculateMaxDihedral(grid, static_cast<Volume*>(prism), aaPos);
}

//	Pyramid
template <class TAAPosVRT>
number CalculateMaxDihedral(Grid& grid, Pyramid* pyr, TAAPosVRT& aaPos)
{
	return CalculateMaxDihedral(grid, static_cast<Volume*>(pyr), aaPos);
}

//	Volume
template <class TAAPosVRT>
number CalculateMaxDihedral(Grid& grid, Volume* v, TAAPosVRT& aaPos)
{
	//PROFILE_FUNC();
//	in the current implementation this method requires, that all edges
//	are created for all faces.
//TODO: improve this!
	if(!grid.option_is_enabled(GRIDOPT_AUTOGENERATE_SIDES))
	{
		LOG("WARNING: autoenabling GRIDOPT_AUTOGENERATE_SIDES in GetNeighbours(Face).\n");
		grid.enable_options(GRIDOPT_AUTOGENERATE_SIDES);
	}

//	Initialization
	uint numElementEdges = v->num_edges();
	vector3 vNorm1, vNorm2;
	number maxDihedral = 0;
	number tmpAngle;
	Face* vNeighbourFacesToEdge[2];

//	Iterate over all element edges
	for(uint eIter = 0; eIter < numElementEdges; ++eIter)
	{
		EdgeBase* e = grid.get_edge(v, eIter);

	//	Get adjacent faces at the current edge and calculate the angle between their normals
		CollectAssociatedSides(vNeighbourFacesToEdge, grid, v, e);

		CalculateNormal(vNorm1, vNeighbourFacesToEdge[0], aaPos);
		CalculateNormal(vNorm2, vNeighbourFacesToEdge[1], aaPos);

	/*	!!!	Beware of the correct direction normals to get correct angle value !!!
		INFO:	Angles of a regular tetrahedron:
				- "Tetrahedron (dihedral) angle x" 	= 109,471...deg
				- "Face-to-face angle y"			= 180deg - x = 70,52...deg

		--> Old version:
		//VecScale(vNorm1, vNorm1, -1);
		//tmpAngle = acos(VecDot(vNorm1, vNorm2));

		--> New version:
			(s.	"Qualit�ts-Metriken und Optimierung von Tetraeder-Netzen",
				 Seminararbeit von Johannes Ahlmann, Universit�t Karlsruhe)
	*/
		tmpAngle = acos(VecDot(vNorm1, vNorm2));
		tmpAngle = PI - tmpAngle;

	//	Check for maximality
		if(tmpAngle > maxDihedral)
		{
			maxDihedral = tmpAngle;
		}
	}

//	Transform maxDihedral from RAD to DEG
	maxDihedral = 180/PI * maxDihedral;

	return maxDihedral;
}


////////////////////////////////////////////////////////////////////////////////////////////
//	CalculateMinTriangleHeight
template <class TAAPosVRT>
number CalculateMinTriangleHeight(Face* face, TAAPosVRT& aaPos)
{
	//PROFILE_FUNC();
	if(face->num_vertices() == 3)
	{
	//	Get type of vertex attachment in aaPos and define it as ValueType
		typedef typename TAAPosVRT::ValueType ValueType;

		number minHeight, tmpMinHeight;
		ValueType v = aaPos[face->vertex(2)];
		ValueType dir;

	//	Calculate start height and set to minHeight
		VecSubtract(dir, aaPos[face->vertex(1)], aaPos[face->vertex(0)]);
		minHeight = DistancePointToRay(	v, aaPos[face->vertex(0)], dir);

		for(uint i = 1; i < 3; ++i)
		{
			v = aaPos[face->vertex((i+2)%3)];
			VecSubtract(dir, aaPos[face->vertex((i+1)%3)], aaPos[face->vertex((i))]);
			tmpMinHeight = DistancePointToRay(v, aaPos[face->vertex(i )], dir);

			if(tmpMinHeight < minHeight)
			{
				minHeight = tmpMinHeight;
			}
		}

		return minHeight;
	}
	else
		UG_ASSERT(false, "Error. Face is not a triangle.");

	return NAN;
}


/*
////////////////////////////////////////////////////////////////////////////////////////////
//	CalculateAspectRatio

//	An unimplemented version, so that a compile error occurs if no overload exists.
template <class TElem, class TAAPosVRT>
number CalculateAspectRatio(Grid& grid, TElem* elem, TAAPosVRT& aaPos);

//	Face (Triangles and Quadrilaterals)
template <class TAAPosVRT>
number CalculateAspectRatio(Grid& grid, Face* face, TAAPosVRT& aaPos)
{
	number AspectRatio;
	number maxEdgeLength;

//	Collect element edges, find longest edge and calculate its length
	vector<EdgeBase*> edges;
	CollectAssociated(edges, grid, face);
	EdgeBase* longestEdge = FindLongestEdge(edges.begin(), edges.end(), aaPos);
	maxEdgeLength = EdgeLength(longestEdge, aaPos);

	switch (face->reference_object_id())
	{
		case ROID_TRIANGLE:
		{
		//	MINHEIGHT / MAXEDGELENGTH
		//  optimal Aspect Ratio of a regular triangle
		//  Q = sqrt(3)/2 * a / a = 0.86602540378444...

		//	Calculate minimal triangle height
			number minTriangleHeight = CalculateMinTriangleHeight(face, aaPos);

		//	Calculate the aspect ratio
			AspectRatio = minTriangleHeight / maxEdgeLength;

			return AspectRatio;
		}

		case ROID_QUADRILATERAL:
		{
		//  AREA / MAXEDGELENGTH

		//	Calculate the element area
			number area = FaceArea(face, aaPos);

		//	Calculate the aspect ratio
			AspectRatio = area / maxEdgeLength;

			return AspectRatio;
		}

		default:
		 	UG_ASSERT(false, "Error. Unknown element type for aspect ratio calculation.");
	}

	return NAN;
}


//	Tetrahedron
template <class TAAPosVRT>
number CalculateAspectRatio(Grid& grid, Tetrahedron* tet, TAAPosVRT& aaPos)
{
	number AspectRatio;
	number maxEdgeLength;

//	Collect element edges, find longest edge and calculate its length
	vector<EdgeBase*> edges;
	CollectAssociated(edges, grid, tet);
	EdgeBase* longestEdge = FindLongestEdge(edges.begin(), edges.end(), aaPos);
	maxEdgeLength = EdgeLength(longestEdge, aaPos);

	//	MINHEIGHT / MAXEDGELENGTH
	// 	optimal Aspect Ratio of a regular tetrahedron
	//	 Q = sqrt(2/3) * a / a = 0.81...

//	Calculate the aspect ratio
	AspectRatio = CalculateTetrahedronAspectRatio(grid, tet, aaPos);

	return AspectRatio;
}

//	Prism
template <class TAAPosVRT>
number CalculateAspectRatio(Grid& grid, Prism* prism, TAAPosVRT& aaPos)
{
	number AspectRatio;
	number maxEdgeLength;

//	Collect element edges, find longest edge and calculate its length
	vector<EdgeBase*> edges;
	CollectAssociated(edges, grid, prism);
	EdgeBase* longestEdge = FindLongestEdge(edges.begin(), edges.end(), aaPos);
	maxEdgeLength = EdgeLength(longestEdge, aaPos);

//  VOLUME / MAXEDGELENGTH

//	Calculate the element volume
	number volume = CalculateVolume(*prism, aaPos);

//	Calculate the aspect ratio
	AspectRatio = volume / maxEdgeLength;

	return AspectRatio;
}

//	Pyramid
template <class TAAPosVRT>
number CalculateAspectRatio(Grid& grid, Pyramid* pyr, TAAPosVRT& aaPos)
{
	number AspectRatio;
	number maxEdgeLength;

//	Collect element edges, find longest edge and calculate its length
	vector<EdgeBase*> edges;
	CollectAssociated(edges, grid, pyr);
	EdgeBase* longestEdge = FindLongestEdge(edges.begin(), edges.end(), aaPos);
	maxEdgeLength = EdgeLength(longestEdge, aaPos);

//  VOLUME / MAXEDGELENGTH

//	Calculate the element volume
	number volume = CalculateVolume(*pyr, aaPos);

//	Calculate the aspect ratio
	AspectRatio = volume / maxEdgeLength;

	return AspectRatio;
}

//	Volume
template <class TAAPosVRT>
number CalculateAspectRatio(Grid& grid, Volume* vol, TAAPosVRT& aaPos)
{
	switch (vol->reference_object_id())
	{
		case ROID_TETRAHEDRON:
		{
			return CalculateAspectRatio(grid, static_cast<Tetrahedron*>(vol), aaPos);
		}

		case ROID_PRISM:
		{
			return CalculateAspectRatio(grid, static_cast<Prism*>(vol), aaPos);
		}

		case ROID_PYRAMID:
		{
			return CalculateAspectRatio(grid, static_cast<Pyramid*>(vol), aaPos);
		}

		default:
		 	UG_ASSERT(false, "Error. Unknown element type for aspect ratio calculation.");
	}

	return NAN;
}
*/


////////////////////////////////////////////////////////////////////////////////////////////
//	CalculateAspectRatio
////////////////////////////////////////////////////////////////////////////////////////////

//	An unimplemented version, so that a compile error occurs if no overload exists.
template <class TElem, class TAAPosVRT>
number CalculateAspectRatio(Grid& grid, TElem* elem, TAAPosVRT& aaPos);

//	Face (Triangles and Constrained Triangles supported)
template <class TAAPosVRT>
number CalculateAspectRatio(Grid& grid, Face* face, TAAPosVRT& aaPos)
{
	//PROFILE_FUNC();
	number AspectRatio;
	number maxEdgeLength;

//	Collect element edges, find longest edge and calculate its length
	vector<EdgeBase*> edges;
	CollectAssociated(edges, grid, face);
	EdgeBase* longestEdge = FindLongestEdge(edges.begin(), edges.end(), aaPos);
	maxEdgeLength = EdgeLength(longestEdge, aaPos);

	switch (face->reference_object_id())
	{
		case ROID_TRIANGLE:
		{
		//	MINHEIGHT / MAXEDGELENGTH
		//  optimal Aspect Ratio of a regular triangle
		//  Q = sqrt(3)/2 * a / a = 0.86602540378444...

		//	Calculate minimal triangle height
			number minTriangleHeight = CalculateMinTriangleHeight(face, aaPos);

		//	Calculate the aspect ratio
			AspectRatio = minTriangleHeight / maxEdgeLength;

			return AspectRatio;
		}

		default:
		{
		 	UG_THROW("Note: Currently only faces of type triangle supported in aspect ratio calculation.");
		 	break;
		}
	}

	return NAN;
}

//	Tetrahedron
template <class TAAPosVRT>
number CalculateAspectRatio(Grid& grid, Tetrahedron* tet, TAAPosVRT& aaPos)
{
	//PROFILE_FUNC();
	number AspectRatio;

	//	MINHEIGHT / MAXEDGELENGTH
	// 	optimal Aspect Ratio of a regular tetrahedron
	//	 Q = sqrt(2/3) * a / a = 0.81...

//	Calculate the aspect ratio
	AspectRatio = CalculateTetrahedronAspectRatio(grid, tet, aaPos);

	return AspectRatio;
}

//	Volume
template <class TAAPosVRT>
number CalculateAspectRatio(Grid& grid, Volume* vol, TAAPosVRT& aaPos)
{
	switch (vol->reference_object_id())
	{
		case ROID_TETRAHEDRON:
		{
			return CalculateAspectRatio(grid, static_cast<Tetrahedron*>(vol), aaPos);
		}

		default:
		{
		 	UG_THROW("Note: Currently only volumes of type tetrahedron supported in aspect ratio calculation.");
		 	break;
		}
	}

	return NAN;
}






////////////////////////////////////////////////////////////////////////////////////////////
//	FindElementWithSmallestMinAngle
template <class TIterator, class TAAPosVRT>
typename TIterator::value_type
FindElementWithSmallestMinAngle(Grid& grid, TIterator elementsBegin, TIterator elementsEnd, TAAPosVRT& aaPos)
{
	//PROFILE_FUNC();
//	if volumesBegin equals volumesBegin, then the list is empty and we can
//	immediately return NULL
	//	if(volumesBegin == volumesBegin)
	//		return NULL;

//	Initializations
	typename TIterator::value_type elementWithSmallestMinAngle = *elementsBegin;
	number smallestMinAngle = CalculateMinAngle(grid, elementWithSmallestMinAngle, aaPos);
	++elementsBegin;

//	compare all volumes and find that one with smallest minAngle
	for(; elementsBegin != elementsEnd; ++elementsBegin)
	{
		typename TIterator::value_type curElement = *elementsBegin;
		number curSmallestMinAngle = CalculateMinAngle(grid, curElement, aaPos);

		if(curSmallestMinAngle < smallestMinAngle)
		{
			elementWithSmallestMinAngle = curElement;
			smallestMinAngle = curSmallestMinAngle;
		}
	}

	return elementWithSmallestMinAngle;
}


////////////////////////////////////////////////////////////////////////////////////////////
//	FindVolumeWithSmallestMinDihedral
template <class TIterator, class TAAPosVRT>
typename TIterator::value_type
FindVolumeWithSmallestMinDihedral(Grid& grid, TIterator elementsBegin, TIterator elementsEnd, TAAPosVRT& aaPos)
{
	//PROFILE_FUNC();
//	if volumesBegin equals volumesBegin, then the list is empty and we can
//	immediately return NULL
	//	if(volumesBegin == volumesBegin)
	//		return NULL;

//	Initializations
	typename TIterator::value_type elementWithSmallestMinDihedral = *elementsBegin;
	number smallestMinDihedral = CalculateMinDihedral(grid, elementWithSmallestMinDihedral, aaPos);
	++elementsBegin;

//	compare all volumes and find that one with smallest minAngle
	for(; elementsBegin != elementsEnd; ++elementsBegin)
	{
		typename TIterator::value_type curElement = *elementsBegin;
		number curSmallestMinDihedral = CalculateMinDihedral(grid, curElement, aaPos);

		if(curSmallestMinDihedral < smallestMinDihedral)
		{
			elementWithSmallestMinDihedral = curElement;
			smallestMinDihedral = curSmallestMinDihedral;
		}
	}

	return elementWithSmallestMinDihedral;
}


////////////////////////////////////////////////////////////////////////////////////////////
//	FindElementWithLargestMaxAngle
template <class TIterator, class TAAPosVRT>
typename TIterator::value_type
FindElementWithLargestMaxAngle(Grid& grid, TIterator elementsBegin, TIterator elementsEnd, TAAPosVRT& aaPos)
{
	//PROFILE_FUNC();
//	if volumesBegin equals volumesBegin, then the list is empty and we can
//	immediately return NULL
	//	if(volumesBegin == volumesBegin)
	//		return NULL;

//	Initializations
	typename TIterator::value_type elementWithLargestMaxAngle = *elementsBegin;
	number largestMaxAngle = CalculateMaxAngle(grid, elementWithLargestMaxAngle, aaPos);
	++elementsBegin;

//	compare all volumes and find that one with largest maxAngle
	for(; elementsBegin != elementsEnd; ++elementsBegin)
	{
		typename TIterator::value_type curElement = *elementsBegin;
		number curLargestMaxAngle = CalculateMaxAngle(grid, curElement, aaPos);

		if(curLargestMaxAngle > largestMaxAngle)
		{
			elementWithLargestMaxAngle = curElement;
			largestMaxAngle = curLargestMaxAngle;
		}
	}

	return elementWithLargestMaxAngle;
}


////////////////////////////////////////////////////////////////////////////////////////////
//	FindVolumeWithLargestMaxDihedral
template <class TIterator, class TAAPosVRT>
typename TIterator::value_type
FindVolumeWithLargestMaxDihedral(Grid& grid, TIterator elementsBegin, TIterator elementsEnd, TAAPosVRT& aaPos)
{
	//PROFILE_FUNC();
//	if volumesBegin equals volumesBegin, then the list is empty and we can
//	immediately return NULL
	//	if(volumesBegin == volumesBegin)
	//		return NULL;

//	Initializations
	typename TIterator::value_type elementWithLargestMaxDihedral = *elementsBegin;
	number largestMaxDihedral = CalculateMaxDihedral(grid, elementWithLargestMaxDihedral, aaPos);
	++elementsBegin;

//	compare all volumes and find that one with largest maxDihedral
	for(; elementsBegin != elementsEnd; ++elementsBegin)
	{
		typename TIterator::value_type curElement = *elementsBegin;
		number curLargestMaxDihedral = CalculateMaxDihedral(grid, curElement, aaPos);

		if(curLargestMaxDihedral > largestMaxDihedral)
		{
			elementWithLargestMaxDihedral = curElement;
			largestMaxDihedral = curLargestMaxDihedral;
		}
	}

	return elementWithLargestMaxDihedral;
}


////////////////////////////////////////////////////////////////////////////////////////////
//	FindLargestFace
template <class TIterator, class TAAPosVRT>
Face* FindLargestFace(TIterator facesBegin, TIterator facesEnd, TAAPosVRT& aaPos)
{
	//PROFILE_FUNC();
	//	if facesBegin equals facesEnd, then the list is empty and we can
	//	immediately return NULL
		if(facesBegin == facesEnd)
			return NULL;

	//	the first face is the first candidate for the smallest face.
		Face* largestFace = *facesBegin;
		number largestArea = FaceArea(largestFace, aaPos);
		++facesBegin;

		for(; facesBegin != facesEnd; ++facesBegin){
			Face* curFace = *facesBegin;
			number curArea = FaceArea(curFace, aaPos);
			if(curArea > largestArea){
				largestFace = curFace;
				largestArea = curArea;
			}
		}

		return largestFace;
}


////////////////////////////////////////////////////////////////////////////////////////////
//	FindSmallestVolumeElement
template <class TIterator, class TAAPosVRT>
typename TIterator::value_type
FindSmallestVolume(TIterator volumesBegin, TIterator volumesEnd, TAAPosVRT& aaPos)
{
	//PROFILE_FUNC();
//	if volumesBegin equals volumesBegin, then the list is empty and we can
//	immediately return NULL
	//	if(volumesBegin == volumesBegin)
	//		return NULL;

//	Initializations
	typename TIterator::value_type smallestVolume = *volumesBegin;
	number smallestVolumeVolume = CalculateVolume(*smallestVolume, aaPos);
	++volumesBegin;

//	compare all tetrahedrons and find minimal volume
	for(; volumesBegin != volumesEnd; ++volumesBegin)
	{
		Volume* curVolume = *volumesBegin;
		number curVolumeVolume = CalculateVolume(*curVolume, aaPos);

		if(curVolumeVolume < smallestVolumeVolume)
		{
			smallestVolume = curVolume;
			smallestVolumeVolume = curVolumeVolume;
		}
	}

	return smallestVolume;
}


////////////////////////////////////////////////////////////////////////////////////////////
//	FindLargestVolumeElement
template <class TIterator, class TAAPosVRT>
typename TIterator::value_type
FindLargestVolume(TIterator volumesBegin, TIterator volumesEnd, TAAPosVRT& aaPos)
{
	//PROFILE_FUNC();
//	if volumesBegin equals volumesBegin, then the list is empty and we can
//	immediately return NULL
	//	if(volumesBegin == volumesBegin)
	//		return NULL;

//	Initializations
	typename TIterator::value_type largestVolume = *volumesBegin;
	number largestVolumeVolume = CalculateVolume(*largestVolume, aaPos);
	++volumesBegin;

//	compare all tetrahedrons and find minimal volume
	for(; volumesBegin != volumesEnd; ++volumesBegin)
	{
		Volume* curVolume = *volumesBegin;
		number curVolumeVolume = CalculateVolume(*curVolume, aaPos);

		if(curVolumeVolume > largestVolumeVolume)
		{
			largestVolume = curVolume;
			largestVolumeVolume = curVolumeVolume;
		}
	}

	return largestVolume;
}


////////////////////////////////////////////////////////////////////////////////////////////
//	FindElementWithSmallestAspectRatio
template <class TIterator, class TAAPosVRT>
typename TIterator::value_type
FindElementWithSmallestAspectRatio(Grid& grid, 	TIterator elemsBegin,
												TIterator elemsEnd, TAAPosVRT& aaPos)
{
	//PROFILE_FUNC();
//	if volumesBegin equals volumesBegin, then the list is empty and we can
//	immediately return NULL
	//	if(volumesBegin == volumesBegin)
	//		return NULL;

//	Initializations
	typename TIterator::value_type elementWithSmallestAspectRatio = *elemsBegin;
	number smallestAspectRatio = CalculateAspectRatio(grid, elementWithSmallestAspectRatio, aaPos);
	++elemsBegin;

//	compare all tetrahedrons and find that one with minimal aspect ratio
	for(; elemsBegin != elemsEnd; ++elemsBegin)
	{
		typename TIterator::value_type curElement = *elemsBegin;
		//TElem* curElement = *elemsBegin;
		number curSmallestAspectRatio = CalculateAspectRatio(grid, curElement, aaPos);

		if(curSmallestAspectRatio < smallestAspectRatio)
		{
			elementWithSmallestAspectRatio = curElement;
			smallestAspectRatio = curSmallestAspectRatio;
		}
	}

	return elementWithSmallestAspectRatio;
}


////////////////////////////////////////////////////////////////////////////////////////////
//	FindElementWithLargestAspectRatio
template <class TIterator, class TAAPosVRT>
typename TIterator::value_type
FindElementWithLargestAspectRatio(Grid& grid,  	TIterator elemsBegin,
												TIterator elemsEnd, TAAPosVRT& aaPos)
{
	//PROFILE_FUNC();
//	if volumesBegin equals volumesBegin, then the list is empty and we can
//	immediately return NULL
	//	if(volumesBegin == volumesBegin)
	//		return NULL;

//	Initializations
	typename TIterator::value_type elementWithLargestAspectRatio = *elemsBegin;
	number largestAspectRatio = CalculateAspectRatio(grid, elementWithLargestAspectRatio, aaPos);
	++elemsBegin;

//	compare all tetrahedrons and find that one with maximal aspect ratio
	for(; elemsBegin != elemsEnd; ++elemsBegin)
	{
		typename TIterator::value_type curElement = *elemsBegin;
		//TElem* curElement = *elemsBegin;
		number curSmallestAspectRatio = CalculateAspectRatio(grid, curElement, aaPos);

		if(curSmallestAspectRatio > largestAspectRatio)
		{
			elementWithLargestAspectRatio = curElement;
			largestAspectRatio = curSmallestAspectRatio;
		}
	}

	return elementWithLargestAspectRatio;
}






////////////////////////////////////////////////////////////////////////////////////////////
//	MinAngleHistogram
template <class TIterator, class TAAPosVRT>
void MinAngleHistogram(Grid& grid, 	TIterator elementsBegin,
									TIterator elementsEnd,
									TAAPosVRT& aaPos,
									uint stepSize)
{
	//PROFILE_FUNC();
//	Initialization
	vector<number> minAngles;
	typename TIterator::value_type refElem = *elementsBegin;

//	Calculate the minAngle of every element
	for(TIterator iter = elementsBegin; iter != elementsEnd; ++iter)
	{
		number curMinAngle = CalculateMinAngle(grid, *iter, aaPos);
		minAngles.push_back(curMinAngle);
	}

//	Sort the calculated minAngles in an ascending way
	sort (minAngles.begin(), minAngles.end());


//	Evaluate the minimal and maximal degree rounding to 10
	int minDeg = round(number(minAngles.front()) / 10.0) * 10;
	int maxDeg = round(number(minAngles.back()) / 10.0) * 10;

//	Expand minDeg and maxDeg by plus minus 10 degrees or at least to 0 or 180 degress
	if((minDeg-10) > 0)
		minDeg = minDeg - 10;
	else
		minDeg = 0;

	if((maxDeg+10) < 180)
		maxDeg = maxDeg + 10;
	else
		maxDeg = 180;

//	Evaluate the number of ranges in respect to the specified step size
	uint numRanges = floor((maxDeg-minDeg) / stepSize);
	vector<uint> counter(numRanges, 0);

//	Count the elements in their corresponding minAngle range
	for(uint i = 0; i < minAngles.size(); ++i)
	{
		number minAngle = minAngles[i];
		for (uint range = 0; range < numRanges; range++)
		{
			if (minAngle < minDeg + (range+1)*stepSize)
			{
				++counter[range];
				break;
			}
		}
	}

//	----------------------------------------
//	Histogram table output section: (THIRDS)
//	----------------------------------------

//	Divide the output table into three thirds (columnwise)
	uint numRows = ceil(number(numRanges) / 3.0);

//	Create table object
	ug::Table<std::stringstream> minAngleTable(numRows, 6);

//	Specific element header
	//UG_LOG(endl << "MinAngle-Histogram for '" << refElem->reference_object_id() << "' elements");
	UG_LOG(endl << "MinAngle-Histogram for '" << refElem->base_object_id() << "d' elements");
	UG_LOG(endl);

//	First third
	uint i = 0;
	for(; i < numRows; ++i)
	{
		minAngleTable(i, 0) << minDeg + i*stepSize << " - " << minDeg + (i+1)*stepSize << " deg : ";
		minAngleTable(i, 1) << counter[i];
	}

//	Second third
//	Check, if second third of table is needed
	if(i < counter.size())
	{
		for(; i < 2*numRows; ++i)
		{
			minAngleTable(i-numRows, 2) << minDeg + i*stepSize << " - " << minDeg + (i+1)*stepSize << " deg : ";
			minAngleTable(i-numRows, 3) << counter[i];
		}
	}

//	Third third
	if(i < counter.size())
//	Check, if third third of table is needed
	{
		for(; i < numRanges; ++i)
		{
			minAngleTable(i-2*numRows, 4) << minDeg + i*stepSize << " - " << minDeg + (i+1)*stepSize << " deg : ";
			minAngleTable(i-2*numRows, 5) << counter[i];
		}
	}

//	Output table
	UG_LOG(endl << minAngleTable);


}


////////////////////////////////////////////////////////////////////////////////////////////
//	ElementQualityStatistics
////////////////////////////////////////////////////////////////////////////////////////////

//	Wrapper for multigrids
//void ElementQualityStatistics(MultiGrid& mg, int level)
void ElementQualityStatistics(MultiGrid& mg)
{
	ElementQualityStatistics(mg, mg.get_geometric_objects());
}

//	Wrapper for grids
void ElementQualityStatistics(Grid& grid)
{
	ElementQualityStatistics(grid, grid.get_geometric_objects());
}


void ElementQualityStatistics(Grid& grid, GeometricObjectCollection goc)
{
	//PROFILE_FUNC();

	Grid::VertexAttachmentAccessor<APosition> aaPos(grid, aPosition);

//	Numbers
	number n_minEdge;
	number n_maxEdge;
	//number n_minFace;
	//number n_maxFace;
	number n_minFaceAngle;
	number n_maxFaceAngle;
	number n_minTriAspectRatio;
	number n_maxTriAspectRatio;

	number n_minVolume;
	number n_maxVolume;
	number n_minVolAngle;
	number n_maxVolAngle;
	number n_minVolDihedral;
	number n_maxVolDihedral;
	number n_minTetAspectRatio;
	number n_maxTetAspectRatio;


//	Elements
	EdgeBase* minEdge;
	EdgeBase* maxEdge;
	//Face* minAreaFace;
	//Face* maxAreaFace;
	Face* minAngleFace;
	Face* maxAngleFace;
	Face* minAspectRatioTri;
	Face* maxAspectRatioTri;

	Volume* minVolume;
	Volume* maxVolume;
	Volume* minAngleVol;
	Volume* maxAngleVol;
	Volume* minDihedralVol;
	Volume* maxDihedralVol;
	Tetrahedron* minAspectRatioTet;
	Tetrahedron* maxAspectRatioTet;


//	Basic grid properties on level i
	UG_LOG(endl << "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%" << endl);
	UG_LOG("GRID QUALITY STATISTICS" << endl << endl);
	for(uint i = 0; i < goc.num_levels(); ++i)
	{
		//PROFILE_BEGIN(eqs_qualityStatistics2d);
	//	----------
	//	2D section
	//	----------
		minEdge = FindShortestEdge(goc.begin<EdgeBase>(i), goc.end<EdgeBase>(i), aaPos);
		maxEdge = FindLongestEdge(goc.begin<EdgeBase>(i), goc.end<EdgeBase>(i), aaPos);
		//minFace =
		//maxFace =
		minAngleFace = FindElementWithSmallestMinAngle(	grid,
														goc.begin<Face>(i),
														goc.end<Face>(i),
														aaPos);
		maxAngleFace = FindElementWithLargestMaxAngle(	grid,
														goc.begin<Face>(i),
														goc.end<Face>(i),
														aaPos);

		n_minEdge = EdgeLength(minEdge, aaPos);
		n_maxEdge = EdgeLength(maxEdge, aaPos);
		n_minFaceAngle = CalculateMinAngle(grid, minAngleFace, aaPos);
		n_maxFaceAngle = CalculateMaxAngle(grid, maxAngleFace, aaPos);

	//	Check for triangles
		if(goc.num<Triangle>(i) > 0)
		{
			minAspectRatioTri = FindElementWithSmallestAspectRatio(	grid,
																	goc.begin<Face>(i),
																	goc.end<Face>(i),
																	aaPos);
			maxAspectRatioTri = FindElementWithLargestAspectRatio(	grid,
																	goc.begin<Face>(i),
																	goc.end<Face>(i),
																	aaPos);

			n_minTriAspectRatio = CalculateAspectRatio(grid, minAspectRatioTri, aaPos);
			n_maxTriAspectRatio = CalculateAspectRatio(grid, maxAspectRatioTri, aaPos);
		}

		//PROFILE_END();

	//	----------
	//	3D section
	//	----------
		if(goc.num<Volume>(i) > 0)
		{
			//PROFILE_BEGIN(eqs_qualityStatistics3d);

			minVolume = FindSmallestVolume(	goc.begin<Volume>(i),
											goc.end<Volume>(i),
											aaPos);

			maxVolume = FindLargestVolume(	goc.begin<Volume>(i),
											goc.end<Volume>(i),
											aaPos);

			minAngleVol = FindElementWithSmallestMinAngle(	grid,
															goc.volumes_begin(i),
															goc.volumes_end(i),
															aaPos);
			maxAngleVol = FindElementWithLargestMaxAngle(	grid,
															goc.volumes_begin(i),
															goc.volumes_end(i),
															aaPos);
			minDihedralVol = FindVolumeWithSmallestMinDihedral(	grid,
																goc.volumes_begin(i),
																goc.volumes_end(i),
																aaPos);
			maxDihedralVol = FindVolumeWithLargestMaxDihedral(	grid,
																goc.volumes_begin(i),
																goc.volumes_end(i),
																aaPos);

			n_minVolume = CalculateVolume(*minVolume, aaPos);
			n_maxVolume = CalculateVolume(*maxVolume, aaPos);
			n_minVolAngle = CalculateMinAngle(grid, minAngleVol, aaPos);
			n_maxVolAngle = CalculateMaxAngle(grid, maxAngleVol, aaPos);
			n_minVolDihedral = CalculateMinDihedral(grid, minDihedralVol, aaPos);
			n_maxVolDihedral = CalculateMaxDihedral(grid, maxDihedralVol, aaPos);


		//	Tetrahedron section
			if(goc.num<Tetrahedron>(i) > 0)
			{
				minAspectRatioTet = FindElementWithSmallestAspectRatio(	grid,
																		goc.begin<Tetrahedron>(i),
																		goc.end<Tetrahedron>(i),
																		aaPos);
				maxAspectRatioTet = FindElementWithLargestAspectRatio(	grid,
																		goc.begin<Tetrahedron>(i),
																		goc.end<Tetrahedron>(i),
																		aaPos);

				n_minTetAspectRatio = CalculateAspectRatio(grid, minAspectRatioTet, aaPos);
				n_maxTetAspectRatio = CalculateAspectRatio(grid, maxAspectRatioTet, aaPos);
			}
		}

		//PROFILE_BEGIN(eqs_qualityStatisticsOutput);

	//	Table summary
		ug::Table<std::stringstream> table(10, 4);
		table(0, 0) << "Number of volumes"; 	table(0, 1) << goc.num_volumes(i);
		table(1, 0) << "Number of faces"; 		table(1, 1) << goc.num_faces(i);
		table(2, 0) << "Number of vertices";	table(2, 1) << goc.num_vertices(i) << endl;

		table(3, 0) << "Shortest edge";	table(3, 1) << n_minEdge;
		table(3, 2) << "Longest edge";	table(3, 3) << n_maxEdge;

		table(4, 0) << "Smallest face angle";	table(4, 1) << n_minFaceAngle;
		table(4, 2) << "Largest face angle";	table(4, 3) << n_maxFaceAngle;

		if(goc.num_volumes(i) > 0)
		{
			table(5, 0) << "Smallest volume";		table(5, 1) << n_minVolume;
			table(5, 2) << "Largest volume";		table(5, 3) << n_maxVolume;
			table(6, 0) << "Smallest volume angle";	table(6, 1) << n_minVolAngle;
			table(6, 2) << "Largest volume angle";	table(6, 3) << n_maxVolAngle;
			table(7, 0) << "Smallest volume dihedral";	table(7, 1) << n_minVolDihedral;
			table(7, 2) << "Largest volume dihedral";	table(7, 3) << n_maxVolDihedral;

			if(goc.num<Triangle>(i) > 0)
			{
				table(8, 0) << "Smallest triangle AR"; table(8, 1) << n_minTriAspectRatio;
				table(8, 2) << "Largest triangle AR"; table(8, 3) << n_maxTriAspectRatio;
			}

			if(goc.num<Tetrahedron>(i) > 0)
			{
				table(9, 0) << "Smallest tetrahedron AR";	table(9, 1) << n_minTetAspectRatio;
				table(9, 2) << "Largest tetrahedron AR";	table(9, 3) << n_maxTetAspectRatio;
			}
		}




	//	Output section
		UG_LOG("+++++++++++++++++" << endl);
		UG_LOG(" Grid level " << i << ":" << endl);
		UG_LOG("+++++++++++++++++" << endl << endl);
		UG_LOG(table);

		PROFILE_BEGIN(eqs_minAngleHistogram);
		if(goc.num_volumes(i) > 0)
		{
			MinAngleHistogram(grid, goc.begin<Volume>(i), goc.end<Volume>(i), aaPos, 10);
		}
		else
		{
			MinAngleHistogram(grid, goc.begin<Face>(i), goc.end<Face>(i), aaPos, 10);
		}

		//PROFILE_END();
		UG_LOG(endl);
	}

	UG_LOG(endl << "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%" << endl << endl);

}



}




