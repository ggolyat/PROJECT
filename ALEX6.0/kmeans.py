## 使用kmean model分好data後輸出成前中後三個.bin
import numpy as np
from sklearn.cluster import KMeans
import threading
import warnings

# Filter out FutureWarnings
warnings.filterwarnings("ignore", category=FutureWarning)

def read_binary_file(filename):
    with open(filename, 'rb') as f:
        data = np.fromfile(f, dtype=np.float64)
    return data

def kmeans_clustering(input_filename, output_filenames):
    data = read_binary_file(input_filename)
    data = data.reshape(-1, 1)

    k = 5
    #print(data.shape)
    #print(f"{input_filename}  here 1")
    kmeans = KMeans(n_clusters=k)
    #print(f"{input_filename}  here 2")
    try:
        kmeans.fit(data)
    except Exception as e:
        print("Error occurred during KMeans fitting:", e)
    #print(f"{input_filename}  here 3")

    cluster_centers = kmeans.cluster_centers_
    #print(f"{input_filename}  here 4")
    sorted_indices = np.argsort(cluster_centers.flatten())
    labels_sorted = np.zeros_like(kmeans.labels_)
    for i, idx in enumerate(sorted_indices):
        labels_sorted[kmeans.labels_ == idx] = i

    clustered_data = np.column_stack((data, labels_sorted))

    clusters = []
    for i in range(k):
        cluster_data = clustered_data[clustered_data[:, 1] == i, 0]
        clusters.append(cluster_data)

    for i, filename in enumerate(output_filenames):
        cluster_data = np.asarray(clusters[i], dtype=np.float64)
        with open(filename, 'wb') as f:
            cluster_data.tofile(f)
    
    print("Cluster centers for", input_filename)
    for i in range(3):
        print("Cluster {}: Center = {}, Size = {}".format(i, cluster_centers[sorted_indices[i]], len(clusters[i])))

def main():
    for i in range(0, 100):
        input_file = "random_data_4_20_{}.bin".format(i)
        output_front = "random_data_4_20_{}_front.bin".format(i)
        output_middle = "random_data_4_20_{}_middle.bin".format(i)
        output_back = "random_data_4_20_{}_back.bin".format(i)

        kmeans_clustering(input_file, [output_front, output_middle, output_back])

    print("All files processed")

if __name__ == "__main__":
    main()
