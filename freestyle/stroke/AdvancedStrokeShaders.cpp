
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

#include "../system/PseudoNoise.h"
#include "../system/RandGen.h"
#include "AdvancedStrokeShaders.h"
#include "StrokeIterators.h"

/////////////////////////////////////////
//
//  CALLIGRAPHICS SHADER
//
/////////////////////////////////////////


CalligraphicShader::CalligraphicShader (real iMinThickness, real iMaxThickness,
					const Vec2f &iOrientation, bool clamp)
  : StrokeShader()
{
  _minThickness=iMinThickness;
  _maxThickness=iMaxThickness;
  _orientation = iOrientation;
  _orientation.normalize();
  _clamp = clamp;
}

float ksinToto=0;

void 
CalligraphicShader::shade(Stroke &ioStroke) const 
{
  Interface0DIterator v;
  Functions0D::VertexOrientation2DF0D fun;
  StrokeVertex* sv;
  for(v=ioStroke.verticesBegin();
      !v.isEnd();
      ++v)
    {
      real thickness;
      Vec2f vertexOri(fun(v));
      Vec2r ori2d(-vertexOri[1], vertexOri[0]);
      ori2d.normalizeSafe();
      real scal = ori2d * _orientation;
      sv = dynamic_cast<StrokeVertex*>(&(*v));
      if (_clamp && (scal<0)) {
	scal=0.0;
	sv->attribute().setColor(1,1,1);
      }
      else {
	scal=fabs(scal);
	sv->attribute().setColor(0,0,0);
      }
      thickness=_minThickness+scal*(_maxThickness-_minThickness);
      if (thickness<0.0)
	thickness=0.0;
      sv->attribute().setThickness(thickness/2.0,thickness/2.0);
    }

}

//void 
//TipRemoverShader::shade(Stroke &ioStroke) const 
//{
//  
//  StrokeInternal::StrokeVertexIterator v, vend;
//  for(v=ioStroke.strokeVerticesBegin(), vend=ioStroke.strokeVerticesEnd();
//      v!=vend;
//      ++v)
//    {
//      if (((*v)->curvilinearAbscissa()<_tipLength) ||
//	  ((*v)->strokeLength()-(*v)->curvilinearAbscissa()<_tipLength))
//      { 
//	  (*v)->attribute().setThickness(0.0, 0.0);
//	  (*v)->attribute().setColor(1,1,1);
//      } 
//    }
//
//}



/////////////////////////////////////////
//
//  SPATIAL NOISE SHADER
//
/////////////////////////////////////////

static const unsigned NB_VALUE_NOISE = 512;

SpatialNoiseShader::SpatialNoiseShader (float ioamount, float ixScale, int nbOctave, 
					bool smooth, bool pureRandom)
  : StrokeShader()
{
  _amount = ioamount;
  if (ixScale==0) _xScale=0;
  else _xScale=1.0/ixScale/real(NB_VALUE_NOISE);
  _nbOctave=nbOctave;
  _smooth=smooth;
  _pureRandom=pureRandom;
}
void
SpatialNoiseShader::shade(Stroke &ioStroke) const 
{
  Interface0DIterator v, v2;
  v=ioStroke.verticesBegin();
  Vec2r p(v->getProjectedX(), v->getProjectedY());
  v2=v; ++v2;
  Vec2r p0(v2->getProjectedX(), v2->getProjectedY());
  p0=p+2*(p-p0);
  StrokeVertex* sv;
  sv = dynamic_cast<StrokeVertex*>(&(*v));
  real initU = sv->strokeLength()*real(NB_VALUE_NOISE);
  if (_pureRandom) initU+=RandGen::drand48()*real(NB_VALUE_NOISE);

  Functions0D::VertexOrientation2DF0D fun;
  while (!v.isEnd())
    {
      sv = dynamic_cast<StrokeVertex*>(&(*v));
      Vec2r p(sv->getPoint());
      Vec2r vertexOri(fun(v));
      Vec2r ori2d(vertexOri[0], vertexOri[1]);
      ori2d = Vec2r(p-p0);
      ori2d.normalizeSafe();

      PseudoNoise mynoise;
      real bruit;

      if (_smooth)
	bruit=mynoise.turbulenceSmooth(_xScale*sv->curvilinearAbscissa()+initU,
				       _nbOctave);
      else
	bruit=mynoise.turbulenceLinear(_xScale*sv->curvilinearAbscissa()+initU,
				       _nbOctave);
      
      Vec2r noise(-ori2d[1]*_amount*bruit,
		  ori2d[0]*_amount*bruit);

      sv->SetPoint(p[0]+noise[0], p[1]+noise[1]);
      p0=p;

      ++v;
    }

}


 
/////////////////////////////////////////
//
//  SMOOTHING SHADER
//
/////////////////////////////////////////


SmoothingShader::SmoothingShader (int ionbIteration, real iFactorPoint, real ifactorCurvature, real iFactorCurvatureDifference,
				  real iAnisoPoint, real iAnisoNormal, real iAnisoCurvature, real iCarricatureFactor) 
  : StrokeShader()
{
  _nbIterations=ionbIteration;
  _factorCurvature=ifactorCurvature;
  _factorCurvatureDifference=iFactorCurvatureDifference;
  _anisoNormal=iAnisoNormal;
  _anisoCurvature=iAnisoCurvature;
  _carricatureFactor=iCarricatureFactor;
  _factorPoint=iFactorPoint;
  _anisoPoint=iAnisoPoint;
}



void
SmoothingShader::shade(Stroke &ioStroke) const 
{
  //cerr<<" Smoothing a stroke  "<<endl;

  Smoother smoother(ioStroke);
  smoother.smooth(_nbIterations, _factorPoint, _factorCurvature, _factorCurvatureDifference, 
		  _anisoPoint, _anisoNormal, _anisoCurvature, _carricatureFactor);
}

// SMOOTHER
////////////////////////////

Smoother::Smoother(Stroke &ioStroke)
{
  _stroke=&ioStroke;

  _nbVertices=ioStroke.vertices_size();
  _vertex=new Vec2r[_nbVertices];
  _curvature=new real[_nbVertices];
  _normal=new Vec2r[_nbVertices];
  StrokeInternal::StrokeVertexIterator v, vend;
  int i=0;
  for(v = ioStroke.strokeVerticesBegin(), vend = ioStroke.strokeVerticesEnd();
      v != vend;
      ++v)
    {
      _vertex[i]=(v)->getPoint();	
      i++;
    }
  Vec2r vec_tmp(_vertex[0]-_vertex[_nbVertices-1]);
  _isClosedCurve = (vec_tmp.norm() < M_EPSILON);

  _safeTest=(_nbVertices>4);
}

void Smoother::smooth(int nbIteration, real iFactorPoint, real ifactorCurvature, real iFactorCurvatureDifference,
		      real iAnisoPoint, real iAnisoNormal, real iAnisoCurvature, real iCarricatureFactor)
{
  _factorCurvature=ifactorCurvature;
  _factorCurvatureDifference=iFactorCurvatureDifference;
  _anisoNormal=iAnisoNormal;
  _anisoCurvature=iAnisoCurvature;
  _carricatureFactor=iCarricatureFactor;
  _factorPoint=iFactorPoint;
  _anisoPoint=iAnisoPoint;

  for (int i=0; i<nbIteration; i++)
    iteration ();
  copyVertices(); 
}

real edgeStopping (real x, real sigma)
{
  if (sigma==0.0) return 1.0;
  return exp(-x*x/(sigma*sigma));
}

void 
Smoother::iteration ()
{
  computeCurvature();
  for (int i=1; i<_nbVertices-1; i++)
    {
      real motionNormal=_factorCurvature*_curvature[i]*
	edgeStopping(_curvature[i], _anisoNormal);

      real diffC1=_curvature[i]-_curvature[i-1];
      real diffC2=_curvature[i]-_curvature[i+1];
      real motionCurvature=edgeStopping(diffC1, _anisoCurvature)*diffC1 +
	edgeStopping(diffC2, _anisoCurvature)*diffC2;//_factorCurvatureDifference;
      motionCurvature*=_factorCurvatureDifference;
      //motionCurvature=_factorCurvatureDifference*(diffC1+diffC2);
      if (_safeTest)
	_vertex[i]=Vec2r(_vertex[i]+(motionNormal+motionCurvature)*_normal[i]);
      Vec2r v1(_vertex[i-1]-_vertex[i]);
      Vec2r v2(_vertex[i+1]-_vertex[i]);
      real d1 = v1.norm();
      real d2 = v2.norm();
      _vertex[i]=Vec2r(_vertex[i]+ 
		       _factorPoint*edgeStopping(d2, _anisoPoint)*(_vertex[i-1]-_vertex[i]) +
		       _factorPoint*edgeStopping(d1, _anisoPoint)*(_vertex[i+1]-_vertex[i]) );
    }

  if (_isClosedCurve)
    {
      real motionNormal=_factorCurvature*_curvature[0]*
	edgeStopping(_curvature[0], _anisoNormal);

      real diffC1=_curvature[0]-_curvature[_nbVertices-2];
      real diffC2=_curvature[0]-_curvature[1];
      real motionCurvature=edgeStopping(diffC1, _anisoCurvature)*diffC1 +
	edgeStopping(diffC2, _anisoCurvature)*diffC2;//_factorCurvatureDifference;
      motionCurvature*=_factorCurvatureDifference;
      //motionCurvature=_factorCurvatureDifference*(diffC1+diffC2);
      _vertex[0]=Vec2r(_vertex[0]+(motionNormal+motionCurvature)*_normal[0]);
      _vertex[_nbVertices-1]=_vertex[0];
    }
}


void 
Smoother::computeCurvature ()
{
  int i;
  Vec2r BA, BC, normalCurvature;
  for (i = 1; i < _nbVertices - 1; i++)
    {
      BA=_vertex[i-1]-_vertex[i];
      BC=_vertex[i+1]-_vertex[i];
      real lba=BA.norm(), lbc=BC.norm();
      BA.normalizeSafe();
      BC.normalizeSafe();
      normalCurvature = BA + BC;

      _normal[i]=Vec2r(-(BC-BA)[1], (BC-BA)[0]);
      _normal[i].normalizeSafe();

      _curvature[i] = normalCurvature * _normal[i];
      if (lba+lbc > M_EPSILON)
	_curvature[i]/=(0.5*lba+lbc);
    }
  _curvature[0]=_curvature[1];
  _curvature[_nbVertices-1]=_curvature[_nbVertices-2];
  Vec2r di(_vertex[1]-_vertex[0]); 
  _normal[0] = Vec2r(-di[1], di[0]);
  _normal[0].normalizeSafe();
  di=_vertex[_nbVertices-1]-_vertex[_nbVertices-2];
  _normal[_nbVertices-1]=Vec2r(-di[1], di[0]);
  _normal[_nbVertices-1].normalizeSafe();

  if (_isClosedCurve)
    {
      BA=_vertex[_nbVertices-2]-_vertex[0];
      BC=_vertex[1]-_vertex[0];
      real lba=BA.norm(), lbc=BC.norm();
      BA.normalizeSafe();
      BC.normalizeSafe();
      normalCurvature = BA + BC;

      _normal[i]=Vec2r(-(BC-BA)[1], (BC-BA)[0]);
      _normal[i].normalizeSafe();

      _curvature[i] = normalCurvature * _normal[i];
      if (lba+lbc > M_EPSILON)
	_curvature[i]/=(0.5*lba+lbc);

      _normal[_nbVertices-1]=_normal[0];
      _curvature[_nbVertices-1]=_curvature[0];
    }
}

void 
Smoother::copyVertices ()
{
  int i=0;
  StrokeInternal::StrokeVertexIterator v, vend;
  for(v=_stroke->strokeVerticesBegin(), vend=_stroke->strokeVerticesEnd();
      v!=vend;
      ++v)
    {
      const Vec2r p0((v)->getPoint());
      const Vec2r p1(_vertex[i]);
      Vec2r p(p0 + _carricatureFactor * (p1 - p0));

      (v)->SetPoint(p[0], p[1]);
      i++;
    }

}

 
/////////////////////////////////////////
//
//  OMISSION SHADER
//
/////////////////////////////////////////

OmissionShader::OmissionShader (real sizeWindow, real thrVari, real thrFlat, real lFlat)
{
  _sizeWindow=sizeWindow;
  _thresholdVariation=thrVari;
  _thresholdFlat=thrFlat;
  _lengthFlat=lFlat;
}

void
OmissionShader::shade(Stroke &ioStroke) const 
{
  Omitter omi(ioStroke);
  omi.omit(_sizeWindow, _thresholdVariation, _thresholdFlat, _lengthFlat);

}


// OMITTER
///////////////////////////

Omitter::Omitter (Stroke &ioStroke)
  :Smoother (ioStroke)
{
  StrokeInternal::StrokeVertexIterator v, vend;
  int i=0;
  for(v=ioStroke.strokeVerticesBegin(), vend=ioStroke.strokeVerticesEnd();
      v!=vend;
      ++v)
    {
      _u[i]=(v)->curvilinearAbscissa();
      i++;
    }

}

void
Omitter::omit (real sizeWindow, real thrVari, real thrFlat, real lFlat)
{
  _sizeWindow=sizeWindow;
  _thresholdVariation=thrVari;
  _thresholdFlat=thrFlat;
  _lengthFlat=lFlat;

  for (int i=1; i<_nbVertices-1; i++)
    {
      if (_u[i]<_lengthFlat) continue;
      // is the previous segment flat?
      int j=i-1; 
      while ((j>=0) && (_u[i]-_u[j]<_lengthFlat))
	{
	  if ((_normal[j] * _normal[i]) < _thresholdFlat)
	    ; // FIXME
	  j--;
	}

    }
}
