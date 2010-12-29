% [contstruct] = readcont(contfile, tstart, tend)
%
%	Parses the header from the contfile to get the sampling rate and the
%	number of samples per buffer and then uses nreadcont to read in the
%	data.
%
%	Returns a continuous data structure
%
%	Note that tstart and tend should be strings (e.g. '10:00');
function [contstruct] = readcont(contfile, tstart, tend)

% open the continuous data file
fid = fopen(contfile);

if (fid == -1)
    error(sprintf('%s cannot be opened', contfile));
end



headerstr = '';
while (~strncmp(headerstr, '%%ENDHEADER', 11))
    headerstr = fgets(fid);
    ind = strfind(headerstr, 'nsamplesperbuf');
    slen = length('nsamplesperbuf');
    if (~isempty(ind))
	nsamplesperbuf = str2num(headerstr((ind+slen):end))
    end
    ind = strfind(headerstr, 'samplingrate');
    slen = length('samplingrate');
    if (~isempty(ind))
	samplingrate = str2num(headerstr((ind+slen):end))
    end
end
fclose(fid);

contstruct = nreadcont(contfile, samplingrate, nsamplesperbuf, tstart, tend);
