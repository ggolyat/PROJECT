# 4.0做的測試:

## 測試新range

### 舊range :

  range1(0.0, 10000.0);	
  
  range2(1000000.0, 1001000.0);  
  
  range3(1000000000.0, 1000010000.0);
  
進行五項測試: 

  1-1: bulkload中
  
  1-2: bulkload前
  
  1-3: bulkload前再bulkload中
  
  1-4: bulkload前再bulkload中再bulkload後
  
  1-5: bulkload全部
  
  
### 新range:

  range1(0.0, 10000.0);	
  
  range2(1000000.0, 10010000.0);
  
  range3(1000000000.0, 1000010000.0);
  
進行五項測試: 

  2-1: bulkload前
  
  2-2: bulkload前再bulkload中
  
  2-3: bulkload前再bulkload中再bulkload後
  
  2-4: bulkload中
  
  2-5: bulkload全部

五項測試的順序不一樣只是我做的順序錯了

