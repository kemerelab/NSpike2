#include <stdio.h>
#include <math.h>
#include "../filtercoeffs.h"

double fX[NFILT];
double fY[NFILT];

double RippleFilter(double d) {
  static int ind = 0;
  double val = 0;
  int jind;
  int i;

  fX[ind] = d;
  fY[ind] = 0;
  for (i = 0; i < NFILT; i++) {
    jind = (ind + i) % NFILT;
    val = val + fX[jind] * fNumerator[i] - 
                fY[jind] * fDenominator[i];
  }
  fY[ind] = val;

  ind--;

  if (ind < 0)
    ind += NFILT;

  return val;
}

int main (void)
{

  FILE *fin, *fout;

  char *infilename = "edata.dat";
  char *outfilename = "filtereddata.dat";

  double d;
  double f;

  int i;


  for (i = 0; i < NFILT; i++) {
    fX[i] = 0.0;
    fY[i] = 0.0;
  }

  fin = fopen(infilename,"r");
  fout = fopen(outfilename,"w");


  while (!feof(fin)) {
    fread(&d,sizeof(double),1,fin);
    f = RippleFilter(d);
    fwrite(&f,sizeof(double),1,fout);

    if (fabs(f) > 1e10)
      break;
  }

  fclose(fin);
  fclose(fout);

  return 0;
}
