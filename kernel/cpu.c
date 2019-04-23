#include <cpu.h>
#include <mm.h>

extern u64 arch_cpuid();

u64 nr_cpus = 0;

int register_cpu_local(struct node *node)
{
	struct cpu *cpu = kmalloc(sizeof(*cpu), GFP_KERNEL);
	if (cpu == NULL)
		return -2;

	cpu->index = nr_cpus++;
	cpu->id = arch_cpuid();
	list_add_tail(&cpu->list, &node->cpu_list);
	return 0;
}

int register_cpu_remote(u64 id, struct node *node)
{
	struct cpu *cpu = kmalloc(sizeof(*cpu), GFP_KERNEL);
	if (cpu == NULL)
		return -2;

	cpu->index = nr_cpus++;
	cpu->id = id;
	cpu->status = 0;
	list_add_tail(&cpu->list, &node->cpu_list);
	return 0;
}


struct cpu *get_cpu()
{
	struct node *node_ptr;
	struct cpu *cpu_ptr;
	u64 current_cpu_id = arch_cpuid();
	list_for_each_entry(node_ptr, &node_list, list) {
		list_for_each_entry(cpu_ptr, &node_ptr->cpu_list, list) {
			if (cpu_ptr->id == current_cpu_id)
				return cpu_ptr;
		}
	}
	return NULL;
}

struct cpu *get_cpu_by_index(int index)
{
	struct node *node_ptr;
	struct cpu *cpu_ptr;

	list_for_each_entry(node_ptr, &node_list, list) {
		list_for_each_entry(cpu_ptr, &node_ptr->cpu_list, list) {
			if (cpu_ptr->index == index)
				return cpu_ptr;
		}
	}
	return NULL;
}


u64 get_irq_stack()
{
	struct cpu *cpu = get_cpu();
	return (u64)cpu->irq_stack;
}

void save_tmp_stack(u64 sp)
{
	struct cpu *cpu = get_cpu();
	cpu->tmp_stack = sp;
}

u64 get_tmp_stack()
{
	struct cpu *cpu = get_cpu();
	return cpu->tmp_stack;
}


