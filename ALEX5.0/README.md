#這次的更新
## Part 1
隨機生成小dataset (100K)，測試不同分類方法:

  用python kmeans分群後存成.bin 再用於c++ ALEX
  
  c++ kmeans 每群移除5%
  
  sort後平分三等份
  
  不分 全部一起放 
  
  
## Part 2
使用先前生成好的100M 80%、40%、30%，測試不同分類方法:
  kmeans k=3 有移除離群值

  kmeans k=3

  平分三等份 (90-10都做了)

  不分 全部一起放 
