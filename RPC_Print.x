struct PrintArgs {
    string User_name<256>;
    string Op_name<15>;
    string File_name<256>;
    string Time<20>;
};

program PRINT_PROG {
    version PRINT_VERS {
        void PRINT_USER_OP(PrintArgs) = 1;
    } = 1;
} = 100471976;