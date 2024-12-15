import random

# Define the number of random pairs you want to generate
num_pairs = 10000

# Define the number of nodes in your graph (adjust this value as needed)
num_nodes = 58228

# Generate random pairs of nodes
node_pairs = set()

while len(node_pairs) < num_pairs:
    source = random.randint(0, num_nodes - 1)
    destination = random.randint(0, num_nodes - 1)
    if source != destination:
        node_pairs.add((source, destination))

# Write the generated pairs to the "path_to_find.log" file
with open("path_to_find.log", "w") as file:
    for pair in node_pairs:
        file.write(f"{pair[0]} {pair[1]}\n")

print(f"Generated {num_pairs} random pairs of nodes and wrote them to path_to_find.log.")
# Define a dictionary to store the degrees of nodes
node_degrees = {}

# Read the data from the "edge_data.txt" file
with open("edges.txt", "r") as file:
    for line in file:
        source, destination = map(int, line.split())
        
        # Update the degree of the source node
        if source in node_degrees:
            node_degrees[source] += 1
        else:
            node_degrees[source] = 1

# Print the degrees of the nodes
degrees = []
for node, degree in node_degrees.items():
    # print(f"Node {node} has a degree of {degree}")
    degrees.append(degree)



import random

# Define the number of nodes in your graph and the total number of partitions
num_nodes = 58228  # Adjust this to the actual number of nodes in your graph
total_partitions = 100
total_landmark_nodes = 100
# Generate 50 random landmark nodes
random_landmark_nodes = random.sample(range(num_nodes), 50)



highest_degree_landmark_nodes = [i for i in sorted(range(num_nodes), key=lambda x: degrees[x], reverse=True)[:50]]
print(highest_degree_landmark_nodes)
# Create a list of all landmark nodes
all_landmark_nodes = random_landmark_nodes + highest_degree_landmark_nodes

# Shuffle the list of all landmark nodes for random assignment
random.shuffle(all_landmark_nodes)
remaining_nodes = list(set(range(num_nodes)) - set(all_landmark_nodes))
random.shuffle(remaining_nodes)
# Partition the nodes and assign each partition to a random landmark node
partitions = [[] for _ in range(total_partitions)]
index = 0
for i in remaining_nodes:

    partition_index = i % total_partitions
    partitions[index].append(i)
    index = index + 1
    if (index == 100):
        index = 0

# Write the assignment to the "landmark.log" file
with open("landmark.log", "w") as file:
    for i, landmark_node in enumerate(all_landmark_nodes):
        assigned_partition = partitions[i]
        file.write(f"{landmark_node} {' '.join(map(str, assigned_partition))}\n")

print("Landmark assignment completed and written to landmark.log.")
