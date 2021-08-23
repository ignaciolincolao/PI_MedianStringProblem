letters=("A" "B" "C" "D" "E" "F" "G" "H" "I" "J" "K" "L" "M" "N" "O" "P" "Q" "R" "S" "T" "U" "V" "W" "X" "Y" "Z")
folders=("L090" "L180" "L270" "L360")
size=("90" "180" "270" "360")

cd ./secuencial_wf_levenshtein

for folder in "${folders[@]}";
do
    for letter in "${letters[@]}";
    do
        ./Release/secuencial_wf_levenshtein.exe $letter $folder
    done
done


cd ../wf_levenshtein
for folder in {0..3};
do
    for letter in "${letters[@]}";
    do
        ./x64/Release/wf_levensthein.exe $letter ${folders[$folder]} ${size[$folder]}
    done
done


