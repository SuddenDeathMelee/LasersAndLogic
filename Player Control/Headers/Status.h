struct Status{	//Status holds timed status conditions
   byte level; //level of effect
   byte index; //Stats[] index
   bool b_db; //buf (1) or debuf (0)
   byte num_updates; //number of times to be updated (num_updates = 2^level initially)
   byte reset_val; //Stats[index], value to be reset once status is over
};

Status current[6]; //indexes 0-2 reserved for debufs, indexes 3-5 reserved for bufs

void pushStat(Status stat){//place status in the current[] array
  char i; //Starting index in current[]
  char j; //ending index in current[]
  if(stat.b_db){ //If status is a buf, must be placed in indexes 0-2 of current[]
    i = 0;
    j = 3;
  }
  else{ //If status is debuf, must be placed in indexes 3-5 of current[]
    i = 3;
    j = 6;
  }
  for(i;i<j;i++){ //Find an empty index to place status
    if(current[i].num_updates==0xFF){
      current[i] = stat;
      break;
    }
  }
}