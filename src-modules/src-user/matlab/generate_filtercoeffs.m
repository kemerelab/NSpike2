
samprate = 1500;
passband = [100 400];

passbandAtten = 0.05;
stopbandAtten = 49;

order = 9;

[b,a] = ellip(order,passbandAtten,stopbandAtten,passband/(samprate/2));

N = length(b);

% a2 = fliplr(a);
% b2 = fliplr(b);

filename = 'filtercoeffs.h';

fid = fopen(filename,'w');

fprintf(fid,'#define NFILT %d\n\n',N);
% fprintf(fid,'/* NOTE THAT COEFFICIENTS ARE REVERSED!!! */\n\n\n');

fprintf(fid,'double fNumerator[NFILT] = {\n');
for i = 1:N-1
  fprintf(fid,'  %25.18e,\n', b(i));
end
fprintf(fid,'  %25.18e};\n\n\n', b(N));

fprintf(fid,'double fDenominator[NFILT] = {\n');
for i = 1:N-1
  fprintf(fid,'  %25.18e,\n', a(i));
end
fprintf(fid,'  %25.18e};\n\n\n', a(N));
fclose(fid);

save ellipfilter b a samprate passband passbandAtten stopbandAtten;

fid = fopen('tempa.txt','w');
fprintf(fid,'  %12.6e\n', a2);
fclose(fid);
fid = fopen('tempb.txt','w');
fprintf(fid,'  %12.6e\n', b2);
fclose(fid);
