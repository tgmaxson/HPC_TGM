import matplotlib.pyplot as plt
import numpy as np

# Data: number of rows for each rank and total columns
rows_per_rank = [13, 13, 12, 12]
columns = 50  # Total columns
colors = ['skyblue', 'lightgreen', 'lightcoral', 'wheat']  # Different color for each rank

# Create a figure and axis for the plot
fig, ax = plt.subplots(figsize=(10, 6))

# Track the starting y-coordinate for each rank
y_offset = 0
for rank, (rows, color) in enumerate(zip(rows_per_rank, colors)):
    # Draw the rectangle representing the data block for each rank
    ax.add_patch(plt.Rectangle((0, y_offset), columns, rows, edgecolor='black', facecolor=color))

    # Add text inside the rectangle to show the (X, Y) size
    text_label = f'Rank {rank} ({columns}, {rows})'
    ax.text(columns / 2, y_offset + rows / 2, text_label, ha='center', va='center', color='black')

    # Update the y_offset for the next rank
    y_offset += rows

# Set the labels and title of the plot
ax.set_xlabel('Columns')
ax.set_ylabel('Rows')
ax.set_title('Distribution of Data Across Ranks')

# Setting the yticks to be at the middle of each rank's block for clarity
ax.set_yticks(np.cumsum(rows_per_rank) - np.array(rows_per_rank) / 2)
ax.set_yticklabels([f'Rank {i}' for i in range(len(rows_per_rank))])
ax.set_xlim(0, columns)
ax.set_ylim(0, sum(rows_per_rank))

# Remove the legend if not needed since each rank is labeled
# ax.legend([f'Rank {i}' for i in range(len(rows_per_rank))], title="Ranks")

plt.savefig("rank_work.png")