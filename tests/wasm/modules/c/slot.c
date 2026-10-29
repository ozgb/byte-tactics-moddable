typedef int (*slot_fn)(int);

static int data_word = 1234;

static int twice(int x)
{
    return x * 2 + data_word;
}

static slot_fn slot = twice;

int *module_data(void)
{
    return &data_word;
}

slot_fn *module_slot(void)
{
    return &slot;
}
