% test of bessel filter for ripple extraction

load fraeeg08-2-29
e = eeg{8}{2}{29};

fid = fopen('edata.dat','w');
fwrite(fid,double(e.data),'double');
fclose(fid);
