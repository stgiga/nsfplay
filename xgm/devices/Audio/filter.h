#ifndef _FILTER_H_
#define _FILTER_H_
#include <math.h>
#include "../device.h"


namespace xgm
{
  class Compressor : public IRenderable
  {
  private:
    INT32 strength;
    INT32 limit;
    INT32 threshold;

  public:
    Compressor()
    {
      SetParam(32767,1.0,1.0);
    }
    ~Compressor()
    {
    }

    void Reset()
    {
    }

    void SetRate(double rate)
    {
    }

    void SetParam(double lim, double thr, double str)
    {
      if(lim<0.0) lim=0.0;
      limit = (lim<1.0)?((INT32)(lim*32767)):0;
      threshold = (INT32)(thr * limit);
      strength =  (INT32)(str * (1<<12));
    }

    inline INT32 Limiter(INT32 in)
    {
      if( threshold < in)
        in = threshold + (((in - threshold) * strength) >> 12);
      else if( in < -threshold )
        in = -threshold + (((in + threshold) * strength) >> 12);

      if(limit)
      {
        if(limit<in)
          in = limit;
        else if(in<-limit)
          in = -limit;
      }

      return in;
    }

    UINT32 Render(INT32 b[2])
    {
      return FastRender(b);
    }

    inline UINT32 FastRender(INT32 b[2])
    {
      b[0] = Limiter(b[0]);
      b[1] = Limiter(b[1]);
      return 2;
    }
  };

  class DCFilter : public IRenderable
  {
  private:
    double dc_weight;
    double in[2], out[2];
    double rate;

  public:
    DCFilter()
    {
      Reset();
    }

    ~DCFilter()
    {
    }

    void SetRate(double r)
    {
      // �J�b�g�I�t���g�� : 2pi*R*C
      rate = r;
      const double C = 10.0E-06, R=270;
      dc_weight = 1.0/(1.0/(rate*R*C)+1.0);
    }

    void SetParam(double R, double C)
    {
      if(C==0.0||R==0.0)
      {
        C=10.0E-06/256;
        R=270;
      }
      dc_weight = 1.0/(1.0/(rate*R*C)+1.0);
    }

    // ��virtual��Render
    inline UINT32 FastRender(INT32 b[2])
    {
      if(dc_weight<1.0)
      {
        out[0] = dc_weight * ( out[0] + b[0] - in [0] );
        in[0] = b[0];
        b[0] = (INT32)out[0];

        out[1] = dc_weight * ( out[1] + b[1] - in [1] );
        in[1] = b[1];
        b[1] = (INT32)out[1];      
      }
      return 2;
    }

    UINT32 Render (INT32 b[2])
    {
      return FastRender(b);
    }

    void Reset()
    {
      in[0] = in[1] = 0;
      out[0] = out[1] = 0;
    }
  };

  class SimpleFIR
  {
  protected:
	  INT32 *tap;
	  double *h;
	  int N,M;
	  double rate;
	  double cutoff;
  public:
	  SimpleFIR(int tap_num);
	  virtual ~SimpleFIR();
	  void SetRate(double r);
	  void SetCutoff(double c);
	  void Reset();
	  void Put(INT32 wav);
	  INT32 Get();
  };
 
  class Filter : public IRenderable
  {
  protected:
    IRenderable * target;

    int type;
    INT32 out[2];
    double a;
    double rate, R, C;
    bool disable;
    const int GETA_BITS;
  public:
    Filter () : GETA_BITS(20)
    {
      target = NULL;
      rate = DEFAULT_RATE;
      R = 4700;
      C = 10.0E-9;
      disable = false;
      out[0]=out[1]=0;
    }

    virtual ~Filter ()
    {
    }

    void Attach (IRenderable * t)
    {
      target = t;
    }

    inline UINT32 FastRender (INT32 b[2])
    {
      if(target) target->Render (b);
      if(a<1.0)
      {
        out[0]+=(INT32)(a*(b[0]-out[0]));
        out[1]+=(INT32)(a*(b[1]-out[1]));
        b[0]=out[0];
        b[1]=out[1];
      }
      return 2;
    }

    UINT32 Render(INT32 b[2])
    {
      return FastRender(b);
    }

    void SetParam (double r, double c)
    {
      C = (1.0E-10) * c;
      R = r;
      UpdateFactor();
    }

    void SetClock (double clock)
    {
      Reset ();
    }

    void SetRate (double r)
    {
      rate = r;
      UpdateFactor();
    }

    void UpdateFactor()
    {
      if (R!=0.0&&C!=0.0&&rate!=0.0)
        a = (1.0/(R * C * rate));
      else
        a = 1.0;      
    }

    void Reset ()
    {
      UpdateFactor();
      out[0] = out[1] = 0;
    }

  };

}                               // namespace

#endif