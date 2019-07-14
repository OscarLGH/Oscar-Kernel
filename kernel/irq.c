#include <irq.h>
#include <cpu.h>

int alloc_irqs_cpu(int cpu, int nr_irq)
{
	struct node *node;
	struct cpu *cpu_ptr;
	int ret;
	list_for_each_entry(node, &node_list, list) {
		list_for_each_entry(cpu_ptr, &node->cpu_list, list) {
			if (cpu_ptr->id == cpu) {
				ret = bitmap_allocate_bits(cpu_ptr->intr_desc.irq_bitmap, nr_irq);
				if (ret != -1) {
					cpu_ptr->intr_desc.irq_nr += nr_irq;
					return ret;
				}
			}
		}
	}
	return -1;
}

int alloc_irqs(int *cpu, int nr_irq, int align)
{
	struct node *node;
	struct cpu *cpu_ptr;
	int ret;
	list_for_each_entry(node, &node_list, list) {
		list_for_each_entry(cpu_ptr, &node->cpu_list, list) {
			/* MSI interrupt can only modified lower n(irq) bit of data, so irq must be aligned .*/
			ret = bitmap_allocate_bits_aligned(cpu_ptr->intr_desc.irq_bitmap, nr_irq, align);
			if (ret != -1) {
				cpu_ptr->intr_desc.irq_nr += nr_irq;
				*cpu = cpu_ptr->id;
				return ret;
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

int alloc_irq_from_smp(int *cpu)
{
	struct node *node;
	struct cpu *cpu_ptr;
	int min_irqs = 0xffff;
	struct cpu *min_irq_cpu;
	list_for_each_entry(node, &node_list, list) {
		list_for_each_entry(cpu_ptr, &node->cpu_list, list) {
			if (cpu_ptr->intr_desc.irq_nr < min_irqs) {
				min_irqs = cpu_ptr->intr_desc.irq_nr;
				min_irq_cpu = cpu_ptr;
			}
		}
	}
	*cpu = min_irq_cpu->id;
	return alloc_irqs_cpu(min_irq_cpu->id, 1);
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

	action = kmalloc(sizeof(*action), GFP_KERNEL);
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

	action = kmalloc(sizeof(*action), GFP_KERNEL);
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


