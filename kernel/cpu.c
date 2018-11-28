#include <cpu.h>
#include <mm.h>

extern u64 arch_cpuid();

int register_cpu_local(struct node *node)
{
	struct cpu *cpu = bootmem_alloc(sizeof(*cpu));
	if (cpu == NULL)
		return -2;

	cpu->id = arch_cpuid();
	list_add_tail(&cpu->list, &node->cpu_list);
	return 0;
}

int register_cpu_remote(u64 id, struct node *node)
{
	struct cpu *cpu = bootmem_alloc(sizeof(*cpu));
	if (cpu == NULL)
		return -2;

	cpu->id = id;
	list_add_tail(&node->cpu_list, &node->cpu_list);
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
