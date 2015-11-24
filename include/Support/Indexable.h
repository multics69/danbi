/*                                                                 --*- C++ -*-
  Copyright (C) 2012 Changwoo Min. All Rights Reserved.

  This file is part of DANBI project. 

  NOTICE: All information contained herein is, and remains the property 
  of Changwoo Min. The intellectual and technical concepts contained 
  herein are proprietary to Changwoo Min and may be covered  by patents 
  or patents in process, and are protected by trade secret or copyright law. 
  Dissemination of this information or reproduction of this material is 
  strictly forbidden unless prior written permission is obtained 
  from Changwoo Min(multics69@gmail.com). 

  Indexable.h -- Indexable class
 */
#ifndef DANBI_INDEXABLE_H
#define DANBI_INDEXABLE_H
namespace danbi {
class Indexable {
protected:
  int Index; 

public:
  /// getter
  inline int getIndex() {
    return Index; 
  }

  /// setter
  inline void setIndex(int Index_) {
    Index = Index_; 
  }
};

} // End of danbi namespace 

#endif 
