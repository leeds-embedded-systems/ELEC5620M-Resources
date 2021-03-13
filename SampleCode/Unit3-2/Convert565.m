%Converts Image to RGB565 C file. Make sure image correct size.
function Convert565(inputFileName, variableName)
    A = imread( inputFileName );
    h = double(A)/255; %Convert from 0-255 to 0-1
    rgb565=uint16( 2048*uint16(h(:,:,1)*31) + ...
                     32*uint16(h(:,:,2)*63) + ...
                        uint16(h(:,:,3)*31))';
    arraySize = numel(rgb565);
    fd=fopen([variableName '.c'],'wt');
    fprintf(fd,['const unsigned short ' variableName ' [%d]={'],arraySize );
    for chunkStart = 1:16:(arraySize-1)
        chunkEnd = chunkStart + min(15,arraySize-1-chunkStart);
        fprintf(fd,'\n   ');
        fprintf(fd,' 0x%04X,',rgb565(chunkStart:chunkEnd));
    end
    if (mod(arraySize,16) == 1)
        fprintf(fd,'\n   ');
    end
    fprintf(fd,' 0x%04X\n};\n',rgb565(end));
    fclose(fd);
end