letters=("A" "B" "C" "D" "E" "F" "G" "H" "I" "J" "K" "L" "M" "N" "O" "P" "Q" "R" "S" "T" "U" "V" "W" "X" "Y" "Z")
folders=("L090" "L180" "L270" "L360")
for folder in "${folders[@]}";
do
    for letter in "${letters[@]}";
    do
        ./Release/secuencial_wf_levenshtein.exe $letter $folder
    done
done



