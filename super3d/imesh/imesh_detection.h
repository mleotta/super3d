// This is brl/bbas/imesh/imesh_detection.h
#ifndef imesh_detection_h_
#define imesh_detection_h_
//:
// \file
// \brief Detect features on meshes
// \author Matt Leotta (mleotta@lems.brown.edu)
// \date May 29, 2008
//
// \verbatim
//  Modifications
//   <none yet>
// \endverbatim


#include "imesh_config.h"
#include "imesh_mesh.h"

#include <vcl_vector.h>

//: Return the indices of half edges that are on the mesh boundary
//  The results are organized into loops
SUPER3D_IMESH_EXPORT
vcl_vector<vcl_vector<unsigned int> >
imesh_detect_boundary_loops(const imesh_half_edge_set& half_edges);


//: Trace half edges that have been selected into loops
//  \return true if all half edges form loops
//  The loops are returned in \param loops as vectors of half edge indices
SUPER3D_IMESH_EXPORT
bool
imesh_trace_half_edge_loops(const imesh_half_edge_set& half_edges,
                            const vcl_vector<bool>& flags,
                            vcl_vector<vcl_vector<unsigned int> >& loops);

//: Return the indices of contour half edges as seen from direction \param dir
//  The results are organized into loops
SUPER3D_IMESH_EXPORT
vcl_vector<vcl_vector<unsigned int> >
imesh_detect_contour_generator(const imesh_mesh& mesh, const vgl_vector_3d<double>& dir);

//: Mark contour half edges as seen from direction \param dir
//  For each contour edge the half edge with the face normal opposing dir is marked
SUPER3D_IMESH_EXPORT
vcl_vector<bool>
imesh_detect_contours(const imesh_mesh& mesh, vgl_vector_3d<double> dir);


//: Return the indices of contour half edges as seen from center of projection \param pt
//  The results are organized into loops
SUPER3D_IMESH_EXPORT
vcl_vector<vcl_vector<unsigned int> >
imesh_detect_contour_generator(const imesh_mesh& mesh, const vgl_point_3d<double>& pt);

//: Mark contour half edges as seen from center of projection \param pt
//  For each contour edge the half edge with the face normal opposing dir is marked
SUPER3D_IMESH_EXPORT
vcl_vector<bool>
imesh_detect_contours(const imesh_mesh& mesh, const vgl_point_3d<double>& pt);


//: Segment the faces into groups of connected components
SUPER3D_IMESH_EXPORT
vcl_vector<vcl_set<unsigned int> >
imesh_detect_connected_components(const imesh_half_edge_set& he);

//: Compute the set of all faces in the same connected component as \a face
SUPER3D_IMESH_EXPORT
vcl_set<unsigned int>
imesh_detect_connected_faces(const imesh_half_edge_set& he, unsigned int face);


#endif // imesh_detection_h_
