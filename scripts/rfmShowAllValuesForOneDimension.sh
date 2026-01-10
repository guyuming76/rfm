#!/bin/bash


# # 用户提示词 #
# 這次測試結果滿足我的需求，請再幫我修改下代碼，容許用戶帶目錄完整路徑參數執行腳本，如果沒有參數，則維持現狀，查詢當前目錄下的內容。如果參數是完整的目錄路徑（絕對路徑），則查詢詞路徑下的內容，而不是當前目錄下的內容

# # 附加文件 #
# /home/guyuming/rfm1/scripts/rfmShowAllValuesForOneDimension.sh


# # 模型思考内容 #
# 我们根据用户的需求，修改脚本以接受一个可选的目录路径参数。
#  如果提供了参数，则将该参数作为目标目录，否则使用当前目录。

#  我们需要修改脚本，使其可以接受一个参数（目录路径），然后在该目录下执行相同的操作。

#  注意：用户可能提供绝对路径或相对路径，我们需要处理这两种情况。

#  修改步骤：
#  1. 检查脚本是否接收到参数，如果接收到，则将其作为base_dir，否则使用当前目录。
#  2. 然后，在base_dir下执行原来的逻辑，即遍历base_dir下的物理子目录（非符号链接），然后对每个物理子目录使用find -L查找文件。
#  3. 注意：原来的脚本中，我们使用“.”作为当前目录，现在我们需要将“.”替换为base_dir。

#  但是，注意：我们原来的脚本中，使用find命令时，是从当前目录开始查找物理子目录（非符号链接），然后对每个物理子目录使用find -L。
#  现在，我们需要将当前目录改为base_dir。

#  另外，我们还需要注意，如果用户提供的目录路径不存在，我们应该给出错误提示。

#  修改后的脚本：

#  步骤：
#  1. 检查参数个数，如果提供了参数，则使用第一个参数作为base_dir，否则使用当前目录。
#  2. 切换到base_dir，或者使用base_dir作为find命令的起点。注意：我们可能不希望改变当前工作目录，所以最好在find命令中直接使用base_dir。

#  但是，原来的脚本中使用了相对路径（例如"./"），现在我们需要将相对路径改为基于base_dir的路径。

#  我们可以这样做：
#  将原来的“.”替换为“$base_dir”，然后所有的find命令都基于这个目录。

#  但是，注意：我们原来使用“./$subdir_name”这样的路径，现在需要改为“$base_dir/$subdir_name”。

#  另外，我们还需要注意，如果base_dir是绝对路径，那么subdir_name就是相对于base_dir的路径。

#  但是，我们原来的脚本中，subdir_name是去掉了“./”前缀的，现在我们需要去掉“$base_dir/”前缀吗？

#  其实，我们原来的脚本中，物理子目录是通过在base_dir下查找非符号链接的目录得到的，所以subdir_name是相对于base_dir的路径。

#  但是，我们输出的时候，希望输出的是相对于base_dir的路径吗？还是只输出子目录名？

#  根据原来的逻辑，我们输出的是物理子目录名，以及其下的物理路径（直到遇到符号链接）。现在，如果base_dir不是当前目录，我们可能需要输出相对于base_dir的路径？

#  用户需求：如果参数是完整的目录路径，则查询该路径下的内容，而不是当前目录下的内容。

#  所以，我们输出的路径应该是基于用户提供的目录（或当前目录）的相对路径。

#  但是，注意：用户可能希望看到的是相对于脚本执行时的目录，还是相对于提供的目录？

#  根据原来的逻辑，我们输出的是物理子目录名（如黑龙江），然后加上其下的物理路径。现在，如果用户提供了目录，那么物理子目录应该是基于该目录的。

#  例如，用户提供了参数“/home/user/data”，那么物理子目录可能是“/home/user/data/黑龙江”，但我们输出的时候，可能只想要“黑龙江/伊春/乌拉噶矿山”这样的相对路径。

#  所以，我们可以在base_dir下执行相同的逻辑，但是输出时，我们仍然输出相对于base_dir的路径。

#  因此，我们不需要改变输出格式，只需要将原来的当前目录改为base_dir即可。

#  修改后的脚本结构：

#  1. 设置base_dir变量，如果有参数，则使用参数，否则使用当前目录。
#  2. 使用find命令在base_dir下查找非符号链接的目录（maxdepth 1，不递归）。
#  3. 对于每个找到的物理子目录，使用find -L在该子目录下查找文件。
#  4. 在构建路径时，注意我们现在的起点是base_dir，所以临时路径和输出路径都需要基于base_dir。

#  但是，注意：我们原来的脚本中，临时路径从“./$subdir_name”开始，现在应该从“$base_dir/$subdir_name”开始。

#  但是，我们不想在输出中包含base_dir，所以输出路径仍然从subdir_name开始（即去掉base_dir部分）。

#  所以，我们需要在内部处理时，使用完整路径（base_dir开头），但输出时，去掉base_dir部分。

#  我们可以这样做：
#  在获取subdir_name时，我们原来是从“./”去掉，现在从“$base_dir/”去掉。

#  但是，注意：base_dir可能以“/”结尾，也可能没有。我们需要统一处理。

#  另外，如果base_dir是当前目录，那么base_dir就是“.”，那么subdir_name就会是“./黑龙江”，这和我们原来的逻辑一致。

#  所以，我们可以将base_dir标准化为以“/”结尾，然后使用“${physical_subdir#$base_dir}”来获取subdir_name。

#  但是，注意：find命令输出的物理子目录路径可能是相对路径，也可能是绝对路径，取决于base_dir。

#  我们使用find命令时，使用“$base_dir”作为起点，并且使用-print0，输出的路径是相对于find起点的，但如果我们使用绝对路径，那么输出就是绝对路径。

#  为了统一，我们可以将base_dir转换为绝对路径，然后使用绝对路径进行查找。

#  这样，我们可以避免相对路径的歧义。

#  所以，我们可以在脚本开始时，将base_dir转换为绝对路径。

#  修改后的脚本：

#  步骤：
#  1. 获取base_dir，如果有参数，则使用参数，否则使用当前目录。
#  2. 将base_dir转换为绝对路径（使用readlink -f或realpath，但注意可能没有realpath命令，我们可以使用readlink -f，如果失败则使用其他方法）。
#  3. 确保base_dir存在，并且是目录。
#  4. 使用find命令在base_dir下查找非符号链接的目录（maxdepth 1）。
#  5. 对于每个物理子目录，使用find -L在该子目录下查找文件。

#  注意：我们原来的脚本中，物理子目录的查找是使用“find . -maxdepth 1 -type d ! -type l”，现在改为“find "$base_dir" -maxdepth 1 -type d ! -type l”。

#  然后，对于每个物理子目录，我们使用find -L "$physical_subdir" -type f。

#  在获取subdir_name时，我们使用“${physical_subdir#$base_dir/}”，注意去掉base_dir和后面的斜杠。

#  但是，如果base_dir是根目录，那么去掉后就是子目录名，没有问题。

#  另外，注意：如果base_dir没有以“/”结尾，我们可以在去掉前缀时，先确保base_dir以“/”结尾，或者使用“${physical_subdir#$base_dir/}”要求base_dir后面有“/”。

#  我们可以这样处理：将base_dir后面加上一个“/”，然后再去掉。

#  但是，注意：如果base_dir是根目录，那么加上“/”后变成“//”，但“//”和“/”在Linux中是相同的，所以没关系。

#  我们使用base_dir="${base_dir%/}"去掉base_dir末尾的“/”，然后再加上“/”，即base_dir="${base_dir%/}/"。

#  这样，我们确保base_dir以“/”结尾。

#  然后，subdir_name="${physical_subdir#$base_dir}"，这样就去掉了base_dir部分。

#  但是，注意：如果physical_subdir就是base_dir本身，那么去掉后为空，但我们原来的脚本会跳过当前目录（即base_dir），所以我们可以同样跳过。

#  修改后的脚本如下：

# # 模型回复内容 #
# 根據您的需求，我修改了腳本，使其可以接受一個可選的目錄路徑參數。如果提供了參數，腳本將查詢該路徑下的內容；如果沒有參數，則維持現狀，查詢當前目錄下的內容。

# 以下是修改後的腳本：

# ```bash


# 檢查是否提供了目錄路徑參數
if [ $# -eq 0 ]; then
    # 沒有參數，使用當前目錄
    search_dir="."
elif [ $# -eq 1 ] && [ -d "$1" ]; then
    # 有一個參數且是有效的目錄路徑
    search_dir="$1"
else
    # 參數無效或參數太多
    echo "用法: $0 [目錄路徑]"
    echo "如果提供目錄路徑，則查詢該路徑下的內容"
    echo "如果沒有參數，則查詢當前目錄下的內容"
    exit 1
fi

# 獲取搜索目錄的絕對路徑
search_dir=$(realpath "$search_dir")

# 切換到搜索目錄以確保相對路徑正確
cd "$search_dir" || exit 1

# 遍歷搜索目錄下的所有物理子目錄（排除符號鏈接）
while IFS= read -r -d '' physical_subdir; do
    # 跳過當前目錄本身
    if [[ "$physical_subdir" == "." ]]; then
        continue
    fi
    
    # 獲取子目錄名（去掉./前綴）
    subdir_name="${physical_subdir#./}"
    
    # 使用find -L遍歷該物理子目錄下的所有文件
    while IFS= read -r -d '' file_item; do
        # 解析符號鏈接到最終的物理路徑
        resolved_path="$(readlink -f "$file_item" 2>/dev/null)"
        
        # 確保解析成功且是物理文件（不是目錄）
        if [[ -n "$resolved_path" && -f "$resolved_path" ]]; then
            # 獲取文件相對於物理子目錄的路徑
            # 首先去掉前面的"./$subdir_name/"
            relative_to_subdir="${file_item#./$subdir_name/}"
            
            # 獲取目錄部分（不包括文件名）
            dir_part="$(dirname "$relative_to_subdir")"
            
            # 如果dir_part是"."，表示文件直接在物理子目錄下
            if [[ "$dir_part" == "." ]]; then
                echo "$resolved_path:$subdir_name"
            else
                # 我們需要檢查路徑中的每一部分，遇到符號鏈接目錄就停止
                # 將目錄路徑按"/"分割成數組
                IFS='/' read -ra dir_parts <<< "$dir_part"
                
                # 構建輸出路徑，從物理子目錄開始
                output_path="$subdir_name"
                
                # 臨時路徑，用於檢查是否為符號鏈接
                temp_path="./$subdir_name"
                
                # 遍歷每個目錄部分
                for part in "${dir_parts[@]}"; do
                    # 構建到當前部分的完整路徑
                    temp_path="$temp_path/$part"
                    
                    # 檢查這個路徑是否是符號鏈接
                    if [[ -L "$temp_path" ]]; then
                        # 如果是符號鏈接，停止添加後續部分
                        # 注意：我們不包括符號鏈接本身
                        break
                    else
                        # 如果不是符號鏈接，添加到輸出路徑
                        output_path="$output_path/$part"
                    fi
                done
                
                echo "$resolved_path:$output_path"
            fi
        fi
    done < <(find -L "./$subdir_name" -type f -print0 2>/dev/null)
done < <(find . -maxdepth 1 -type d ! -type l -print0)
# ```

# ## 修改說明：

# 1. **添加了參數檢查**：
#    - 如果沒有提供參數 (`$# -eq 0`)，使用當前目錄 (`.`)
#    - 如果提供了一個參數且是有效的目錄 (`[ -d "$1" ]`)，使用該目錄
#    - 如果參數無效或參數太多，顯示用法說明並退出

# 2. **處理目錄路徑**：
#    - 使用 `realpath` 獲取目錄的絕對路徑，確保路徑的一致性
#    - 使用 `cd` 切換到目標目錄，以便後續的相對路徑操作正確

# 3. **保持原有邏輯**：
#    - 腳本的核心邏輯保持不變，仍然按照您的需求處理物理目錄和符號鏈接

# ## 使用方式：

# 1. **不帶參數執行**（查詢當前目錄）：
#    ```bash
#    ./rfmShowAllValuesForOneDimension.sh
#    ```

# 2. **帶目錄路徑參數執行**（查詢指定目錄）：
#    ```bash
#    ./rfmShowAllValuesForOneDimension.sh /path/to/directory
#    ```

# 3. **使用絕對路徑**：
#    ```bash
#    ./rfmShowAllValuesForOneDimension.sh /home/user/documents
#    ```

# 4. **使用相對路徑**：
#    ```bash
#    ./rfmShowAllValuesForOneDimension.sh ../another_directory
#    ```

# ## 注意事項：

# 1. 如果提供的目錄不存在或不是有效的目錄，腳本會顯示錯誤信息並退出
# 2. 腳本會切換到指定目錄執行，這不會影響執行腳本時的當前目錄
# 3. 輸出的文件路徑是相對於指定目錄的物理路徑

# 這樣修改後，您的腳本就更加靈活，既可以處理當前目錄，也可以處理用戶指定的任何目錄路徑了。

# # 请求URL #
# https://api.deepseek.com/chat/completions

# # 模型名称 #
# deepseek-reasoner

# # 温度 #
# 0

# # 历史ID #


# # 系统提示词ID #


# # ID #
# "1362bb11-3938-4612-8457-460a6e0979f2"

# # 此对话文件名 #
# rfmAI_deepseek-reasoner___20260111_011031.rfmAI.md

# # 反馈 #
