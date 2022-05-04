LongTime = LongTime;
ShortTime = (0:10:300)';
Matches = zeros(length(ShortTime),1);
ShortHR = zeros(length(ShortTime),1);

 for i = 1: length(ShortTime)
     lowestDiff = 1000;
     for j = 1:length(LongTime)
        difference = abs(ShortTime(i) - LongTime(j));
        if difference < lowestDiff
            lowestDiff = difference;
            Matches(i) = j;
        end
          
    end
 end
 
 for i = 1: length(ShortTime)
     ShortHR(i) = LongHR(Matches(i));
 end