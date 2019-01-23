struct sort_in {
    char p_name[50];
    char args[250]
};

program RPC_EXEC {
 version EXEC{
  int RPC_EXEC (args) = 1;
 }=1;
}= 0x31240088;
