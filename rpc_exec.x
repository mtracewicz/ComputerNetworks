struct in_args {
    char p_name[50];
    char args[250];
    int number_of_arguments;
    int flag;
    char buf[1024];
};

program RPC_EXEC {
 version EXEC{
  int MY_EXEC (in_args) = 1;
 }=1;
}= 0x31240055;
