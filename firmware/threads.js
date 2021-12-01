function decode_stack(sp)
{
  var i;
  var a = new Array();
  var raw = new Array();
  
  a[13] = sp + 68; //R13

  for (i = 0; i <= 17; i++) {
    raw[i] = Debug.evaluate("*(unsigned long*)" + sp);
    sp += 4;
  }
  a[0] = raw[9];  //R0
  a[1] = raw[10]; //R1
  a[2] = raw[11]; //R2
  a[3] = raw[12]; //R3
  a[4] = raw[0];  //R4
  a[5] = raw[1];  //R5
  a[6] = raw[2];  //R6
  a[7] = raw[3];  //R7
  a[8] = raw[4];  //R8
  a[9] = raw[5];  //R9
  a[10] = raw[6]; //R10
  a[11] = raw[7]; //R11
  a[12] = raw[13];//R12

  a[14] = raw[14];//R14
  a[15] = raw[15];//R15
  a[16] = raw[16];//APSR
  return a;
}

function add_task(task, state)
{
  var tcb, task_name, current_task, regs;

  current_task = Debug.evaluate("pxCurrentTCB");
  tcb = Debug.evaluate("*(tskTCB *)" + task);

  task_name = Debug.evaluate("(char*)&(*(tskTCB *)" + task + ").pcTaskName[0]");
  task_name = "#" + tcb.uxTCBNumber + " \"" + task_name + "\"";

  if (task == current_task)
  {
    state = "executing";
    regs = [];
  }
  else
  {
    regs = decode_stack(tcb.pxTopOfStack);
  }

  Threads.add(task_name, tcb.uxPriority, state, regs);
}

function add_list(list, state)
{
  var i, index, item, task;

  if (list && list.uxNumberOfItems>0)
  {
    index = list.xListEnd.pxNext;

    for (i = 0; i < list.uxNumberOfItems; i++)
    {
      item = Debug.evaluate("*(ListItem_t *)" + index);

      task = item ? item.pvOwner : 0;

      if (task)
        add_task(task, state);

      index = item.pxNext;
    }
  }
}

function update() 
{
  var i, list, lists, max_priority;

  Threads.clear();

  if( Debug.evaluate("pxCurrentTCB") == 0 )
    return;

  max_priority = Debug.evaluate("uxTopUsedPriority");

  Threads.newqueue("Ready");
  for (i = max_priority; i >= 0; i--)
  {
     list = Debug.evaluate("pxReadyTasksLists[" + i + "]");
     add_list(list, "ready");
  }

  Threads.newqueue("Blocked");
  list = Debug.evaluate("pxDelayedTaskList");
  if (list)
  {
    list = Debug.evaluate("*(xList *)" + list);
    add_list(list, "blocked");
  }

  list = Debug.evaluate("xDelayedTaskList1");
  if (list) {
    add_list(list, "blocked");
  }

  list = Debug.evaluate("xDelayedTaskList2");
  if (list) {
    add_list(list, "blocked");
  }

  list = Debug.evaluate("pxOverflowDelayedTaskList");
  if (list)
  {
    list = Debug.evaluate("*(xList *)" + list);
    add_list(list, "blocked");
  }

  list = Debug.evaluate("xPendingReadyList");
  if (list) {
    list = Debug.evaluate("*(xList *)" + list);
    add_list(list, "blocked");
  }

  Threads.newqueue("Suspended");
  list = Debug.evaluate("xSuspendedTaskList");
  if (list)
  {
    add_list(list, "suspended");
  }

}