#include <irq.h>
#include <cpu.h>

int alloc_irq(int cpu)
{
	struct node *node;
	struct cpu *cpu_ptr;
	list_for_each_entry(node, &node_list, list) {
		list_for_each_entry(cpu_ptr, &node->cpu_list, list) {
			if (cpu_ptr->id == cpu) {
				cpu_ptr->intr_desc.irq_nr++;
				return bitmap_allocate_bits(cpu_ptr->intr_desc.irq_bitmap, 1);
			}
		}
	}
	return -1;
}

int free_irq(int cpu, int irq)
{
	struct node *node;
	struct cpu *cpu_ptr;
	list_for_each_entry(node, &node_list, list) {
		list_for_each_entry(cpu_ptr, &node->cpu_list, list) {
			if (cpu_ptr->id == cpu) {
				cpu_ptr->intr_desc.irq_nr--;
				bitmap_clear(cpu_ptr->intr_desc.irq_bitmap, irq);
				return 0;
			}
		}
	}
	return -1;
}

int request_irq(int irq, irq_handler_t handler, unsigned long flags, char *name, void *dev)
{
	struct node *node;
	struct cpu *cpu_ptr;
	struct cpu *min_irq_load_cpu;
	struct irq_action *action;
	int min_irq = 0x1000;
	list_for_each_entry(node, &node_list, list) {
		list_for_each_entry(cpu_ptr, &node->cpu_list, list) {
			if (cpu_ptr->intr_desc.irq_nr <= min_irq) {
				min_irq_load_cpu = cpu_ptr;
				min_irq = cpu_ptr->intr_desc.irq_nr;
			}
		}
	}

	action = bootmem_alloc(sizeof(*action));
	if (action == NULL)
		return -1;

	action->irq_handler = handler;
	action->data = dev;
	action->name = name;
	INIT_LIST_HEAD(&action->list);
	min_irq_load_cpu->intr_desc.irq_desc[irq].count++;
	list_add_tail(&action->list, &min_irq_load_cpu->intr_desc.irq_desc[irq].irq_action_list);

	return 0;
}

int request_irq_smp(struct cpu *cpu_ptr, int irq, irq_handler_t handler, unsigned long flags, char *name, void *dev)
{
	struct irq_action *action;

	action = bootmem_alloc(sizeof(*action));
	if (action == NULL)
		return -1;

	action->irq_handler = handler;
	action->data = dev;
	action->name = name;
	INIT_LIST_HEAD(&action->list);
	cpu_ptr->intr_desc.irq_desc[irq].count++;
	list_add_tail(&action->list, &cpu_ptr->intr_desc.irq_desc[irq].irq_action_list);

	return 0;
}


