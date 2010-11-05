% test of bessel filter for ripple extraction

load fraeeg08-2-29
e = eeg{8}{2}{29};
etimes = e.starttime + ((0:(length(e.data(:,1))-1))*(1/e.samprate));

tmpeeg = e.data(1:100000);

load ripplefilter.mat
a = ripplefilter.tf.den;
b = ripplefilter.tf.num;

% note that zero padding is essential
y = filter(b,a,double(e.data));

% b2 = textread('tempb.txt'); a2 = textread('tempa.txt');
load ellipfilter
y2 = filter(b,a,double(e.data));

fid = fopen('../cfilter/filtereddata.dat','r');
ff = fread(fid,inf,'double');

return;

thresh = 3;
lastval = zeros(20,1);
tmpy = abs(y);
v = zeros(size(y));
m = zeros(size(y));
s = zeros(size(y));
rt = [];
i = 2;
while (i <= length(y))
  dm = tmpy(i) - m(i-1);
  m(i) = m(i-1) + dm * 0.001;
  ds = abs(tmpy(i) - m(i)) - s(i-1);
  s(i) = s(i-1) + ds * 0.001;

  posgain = mean(lastval);
  lastval(1:19) = lastval(2:20);
  df = tmpy(i) - v(i-1);

  if (df > 0)
    gain = 1.2;
    v(i) = v(i-1) + df * posgain;
  else
    gain = 0.2;
    v(i) = v(i-1) + df * gain;
  end
  lastval(20) = gain;
  threshtmp = m(i) + thresh * s(i);
  if ((i > 10000) && (v(i) > threshtmp))
    rt = [rt i];
    tmpm = m(i);
    tmps = s(i);
    tmpy(i:i+149) = 0;
    i = i + 149;
    m(i) = tmpm;
    s(i) = tmps;
  end
  i = i + 1;
end

