//
//  Copyright (C) : Please refer to the COPYRIGHT file distributed 
//   with this source distribution. 
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; either version 2
//  of the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//
///////////////////////////////////////////////////////////////////////////////

#include "WXEdgeBuilder.h"
#include "WXEdge.h"

void WXEdgeBuilder::visitIndexedFaceSet(IndexedFaceSet& ifs)
{
  //      printf("wx::visitIndexedFaceSet check:\n");
  //      printf("%d %d %d %d\n",ifs.faceUserData()[0],ifs.faceUserData()[1],ifs.faceUserData()[2],ifs.faceUserData()[3]);

  WXShape *shape = new WXShape;
  buildWShape(*shape, ifs);
  shape->SetId(ifs.getId().getFirst());
  shape->SetMeshSilhouettes(ifs.meshSilhouettes());
  //ifs.SetId(shape->GetId());
}

void WXEdgeBuilder::buildWVertices(WShape& shape,
                   const real *vertices, const real *normals,
			       unsigned vsize, const float * vertexUserData) {
  WXVertex *vertex;
  for (unsigned i = 0; i < vsize; i += 3) {
      vertex = new WXVertex(Vec3r(vertices[i],
                                  vertices[i + 1],
                                  vertices[i + 2]));
      vertex->SetId(i / 3);
      vertex->SetSurfaceNdotV(vertexUserData[i/3]);
      vertex->SetNormal(Vec3r(normals[i],
                              normals[i + 1],
                              normals[i + 2]));
      shape.AddVertex(vertex);
  }
}
