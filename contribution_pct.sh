#!/bin/bash
# 统计提交数 + 提交占比
echo "==== 提交数 & 提交占比 ===="
total_commits=$(git shortlog -sn | awk '{sum+=$1} END{print sum}')
git shortlog -sn | awk -v total="$total_commits" '{
    pct = ($1 / total) * 100;
    printf("%-15s 提交数：%-6d 占比：%.1f%%\n", $2, $1, pct);
}'

# 统计代码行数 + 新增行数占比
echo -e "\n==== 代码行数 & 新增占比 ===="
git log --shortstat --pretty="%an" | awk '
    BEGIN { person=""; add=0; del=0; }
    $0 ~ /^[^ ]/ {
        if (person != "") { authors[person]["a"]+=add; authors[person]["d"]+=del; }
        person=$0; add=0; del=0;
    }
    /insert/ { add+=$1; } /delet/ { del+=$1; }
    END {
        total_add=0; for (a in authors) total_add+=authors[a]["a"];
        for (a in authors) {
            pct=(authors[a]["a"]/total_add)*100;
            printf("%-15s 新增：%-6d 删除：%-6d 新增占比：%.1f%%\n", a, authors[a]["a"], authors[a]["d"], pct);
        }
    }
'
