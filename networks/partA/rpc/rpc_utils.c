int check_draw(int a, int b)
{
    if (a == b)
    {
        printf("Draw\n");
        return 3;
    }

    return 0;
}

int check_win(int a, int b)
{
    if ((a == 0 && b == 2) || (a == 1 && b == 0) || (a == 2 && b == 1))
    {
        return 4;
    }
    else
    {
        return 5;
    }
}

int compute_result(int a, int b)
{

    if (check_draw(a, b) == 3)
    {
        return 3;
    }
    if (check_win(a, b) == 4)
    {
        return 4;
    }
    if (check_win(a, b) == 5)
    {
        return 5;
    }
}
